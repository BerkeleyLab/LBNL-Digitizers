// Bunch current monitor data acquisition
// Nets with names beginning with sys are in the system clock domain.
// Nets with names beginning with evr are in the EVR clock domain.

module acquisitionBCM #(
    parameter CHANNEL_COUNT              = -1,
    parameter SAMPLE_CAPACITY            = -1,
    parameter MAX_PASSES_PER_ACQUISITION = -1,
    parameter AXI_SAMPLES_PER_CLOCK      = -1,
    parameter AXI_SAMPLE_WIDTH           = -1,
    parameter ADC_WIDTH                  = -1
    ) (
    input                     sysClk,
    input                     sysCsrStrobe,
    input                     sysAddrStrobe,
    input              [31:0] GPIO_OUT,
    output wire        [31:0] sysStatusReg,
    output wire signed [31:0] sysReadoutReg,
    output reg         [63:0] triggerTimestamp,

    input        evrClk,
    input        evrPostCycleTrigger,
    input        evrInjectionTrigger,
    input [63:0] evrTimestamp,

    input                                                              adcClk,
    input [(CHANNEL_COUNT*AXI_SAMPLES_PER_CLOCK*AXI_SAMPLE_WIDTH)-1:0] axiData
    );


localparam PASS_COUNT_WIDTH = $clog2(MAX_PASSES_PER_ACQUISITION) + 1;
localparam DPRAM_WIDTH = ADC_WIDTH + $clog2(MAX_PASSES_PER_ACQUISITION);
localparam DPRAM_ADDRESS_WIDTH = $clog2(SAMPLE_CAPACITY/AXI_SAMPLES_PER_CLOCK);
localparam SAMPLE_INDEX_WIDTH = $clog2(AXI_SAMPLES_PER_CLOCK);
localparam CHANNEL_INDEX_WIDTH = $clog2(CHANNEL_COUNT);
localparam ADC_SHIFT = AXI_SAMPLE_WIDTH - ADC_WIDTH;

//////////////////////////////////////////////////////////////////////////////
// System clock domain

reg         [DPRAM_ADDRESS_WIDTH-1:0] sysAddress;
reg         [DPRAM_ADDRESS_WIDTH-1:0] sysAcqCountReload;
reg            [PASS_COUNT_WIDTH-1:0] sysPassCountReload;
reg          [SAMPLE_INDEX_WIDTH-1:0] sysSampleIndex;
reg         [CHANNEL_INDEX_WIDTH-1:0] sysChannelIndex;
reg sysAcqToggle = 0, acqAcqMatch = 0;
wire sysAcqMatch;
reg sysAcqActive = 0;
reg sysSoftTrigger = 0;
toClk acqMatch (.clk(sysClk), .I(acqAcqMatch), .O(sysAcqMatch));

always @(posedge sysClk) begin
    if (sysAcqActive) begin
        if (sysAcqToggle == sysAcqMatch) begin
            sysAcqActive <= 0;
        end
        if (sysCsrStrobe && GPIO_OUT[30]) begin
            sysSoftTrigger <= 1;
        end
    end
    else begin
        if (sysCsrStrobe && GPIO_OUT[31]) begin
            sysSoftTrigger <= 0;
            sysPassCountReload<=GPIO_OUT[DPRAM_ADDRESS_WIDTH+:PASS_COUNT_WIDTH];
            sysAcqCountReload <= GPIO_OUT[0+:DPRAM_ADDRESS_WIDTH];
            sysAcqToggle <= !sysAcqToggle;
            sysAcqActive <= 1;
        end
    end
    if (sysAddrStrobe) begin
        sysSampleIndex <= GPIO_OUT[0+:SAMPLE_INDEX_WIDTH];
        sysAddress <= GPIO_OUT[SAMPLE_INDEX_WIDTH+:DPRAM_ADDRESS_WIDTH];
        sysChannelIndex <= GPIO_OUT[24+:CHANNEL_INDEX_WIDTH];
    end
end

reg acqFollowsInjection = 0;
assign sysStatusReg = { sysAcqActive, sysSoftTrigger, acqFollowsInjection,
                        {32-3-PASS_COUNT_WIDTH-DPRAM_ADDRESS_WIDTH{1'b0}},
                        sysPassCountReload, sysAcqCountReload };

//////////////////////////////////////////////////////////////////////////////
// ADC clock domain

wire acqAcqToggle, acqEVRtrigger, acqSoftTrigger;
toClk acqTog(.clk(adcClk), .I(sysAcqToggle),        .O(acqAcqToggle));
toClk acqEvt(.clk(adcClk), .I(evrPostCycleTrigger), .O_RISE(acqEVRtrigger));
toClk acqSft(.clk(adcClk), .I(sysSoftTrigger),      .O_RISE(acqSoftTrigger));

reg [DPRAM_ADDRESS_WIDTH-1:0] acqCountReload;
reg [DPRAM_ADDRESS_WIDTH-1:0] acqAddressCounter;
reg [DPRAM_ADDRESS_WIDTH-1:0] acqAddress;
reg [DPRAM_ADDRESS_WIDTH-1:0] acqAddress_d1;
reg [DPRAM_ADDRESS_WIDTH-1:0] acqAddress_d2;
reg [DPRAM_ADDRESS_WIDTH-1:0] acqAddress_d3;
reg   [DPRAM_ADDRESS_WIDTH:0] sampleCounter;
wire passDone = sampleCounter[DPRAM_ADDRESS_WIDTH];
reg               [PASS_COUNT_WIDTH-1:0] passCounter;
wire acquisitionDone = passCounter[PASS_COUNT_WIDTH-1];
reg starting = 0;
reg acquiring = 0, acquiring_d1 = 0;
reg acquiring_d2 = 0, acquiring_d3 = 0;
reg firstPass, firstPass_d1;
reg acqUseAddressCounter = 0;

always @(posedge adcClk) begin
    acquiring_d1 <= acquiring;
    acquiring_d2 <= acquiring_d1;
    acquiring_d3 <= acquiring_d2;
    firstPass_d1 <= firstPass;
    acqAddress <= acqUseAddressCounter ? acqAddressCounter : sysAddress;
    acqAddress_d1 <= acqAddress;
    acqAddress_d2 <= acqAddress_d1;
    acqAddress_d3 <= acqAddress_d2;

    /*
     * Acquisition state machine
     * Acquisition sequence begins when these constraints have been met:
     *   1. Arm (acqAcqToggle != acqAcqMatch)
     *   2. Trigger (acqEVRtrigger || acqSoftTrigger)
     */
    if (passDone) begin
        sampleCounter <= { 1'b0, acqCountReload };
        acqAddressCounter <= 0;
    end
    else begin
        sampleCounter <= sampleCounter - 1;
        acqAddressCounter <= acqAddressCounter + 1;
    end
    if (acquiring) begin
        if (passDone) begin
            firstPass <= 0;
            passCounter <= passCounter - 1;
            if (acquisitionDone) begin
                acqUseAddressCounter <= 0;
                acquiring <= 0;
                acqAcqMatch <= !acqAcqMatch;
            end
        end
    end
    else if (starting) begin
        if (passDone) begin
            starting <= 0;
            firstPass <= 1;
            acqFollowsInjection <= evrInjectionTrigger;
            acquiring <= 1;
        end
    end
    else begin
        passCounter       <= sysPassCountReload;
        acqCountReload    <= sysAcqCountReload;
        if ((acqAcqToggle != acqAcqMatch)
         && (acqEVRtrigger || acqSoftTrigger)) begin
            starting <= 1;
            acqUseAddressCounter <= 1;
        end
    end
end

// Simple single-clock dual-port RAM
reg  [(CHANNEL_COUNT*AXI_SAMPLES_PER_CLOCK*DPRAM_WIDTH)-1:0] dpram
                                                 [0:(1<<DPRAM_ADDRESS_WIDTH)-1];
reg  [(CHANNEL_COUNT*AXI_SAMPLES_PER_CLOCK*DPRAM_WIDTH)-1:0] dpramQ;
wire [(CHANNEL_COUNT*AXI_SAMPLES_PER_CLOCK*DPRAM_WIDTH)-1:0] dpramWriteData;
always @(posedge adcClk) begin
    dpramQ <= dpram[acqAddress];
    if (acquiring_d3) dpram[acqAddress_d3] <= dpramWriteData;
end

genvar i;
generate
for (i = 0 ; i < (CHANNEL_COUNT * AXI_SAMPLES_PER_CLOCK) ; i = i + 1) begin
 // ADC values are left adjusted in the AXI_SAMPLE_WIDTH fields.
 wire signed   [ADC_WIDTH-1:0] adcData =
                            axiData[(i*AXI_SAMPLE_WIDTH)+ADC_SHIFT+:ADC_WIDTH];
 reg  signed [DPRAM_WIDTH-1:0] wideAdcData, oldValue, newValue;
 wire signed [DPRAM_WIDTH-1:0] ramValue = dpramQ[i*DPRAM_WIDTH+:DPRAM_WIDTH];
 assign dpramWriteData[i*DPRAM_WIDTH+:DPRAM_WIDTH] = newValue;

 always @(posedge adcClk) begin
     wideAdcData <= adcData;
     oldValue <= firstPass_d1 ? {DPRAM_WIDTH{1'b0}} : ramValue;
     newValue <= oldValue + wideAdcData;
 end
end
endgenerate

// Multiplex into readout register
// Assumes that there's enough time between the processor writing the readout
// address and reading the value for all clock crossing to stabilize.
reg [DPRAM_WIDTH-1:0] dpramMUX;
always @(posedge sysClk) begin
    dpramMUX <= dpramQ[((sysChannelIndex * AXI_SAMPLES_PER_CLOCK) +
                                    sysSampleIndex) * DPRAM_WIDTH+:DPRAM_WIDTH];
end
assign sysReadoutReg = $signed(dpramMUX) << ADC_SHIFT;

//////////////////////////////////////////////////////////////////////////////
// EVR clock domain
wire evrTrigger;
toClk evrTr(.clk(evrClk), .I(acquiring), .O_RISE(evrTrigger));
always @(posedge evrClk) begin
    if (evrTrigger) begin
        triggerTimestamp <= evrTimestamp;
    end
end

endmodule

//////////////////////////////////////////////////////////////////////////////
// Get signal into specified clock domain
// Provide the signal, a rising-edge indicator, and a change-of-state indicator.
module toClk (
    input      clk,
    input      I,
    output reg O,
    output reg O_COS,
    output reg O_RISE);

(* ASYNC_REG = "TRUE" *) reg I_m;
reg I_s;

always @(posedge clk) begin
    I_m <= I;
    I_s <= I_m;
    O   <= I_s;
    O_COS <= I_s != O;
    O_RISE <= I_s && !O;
end
endmodule
