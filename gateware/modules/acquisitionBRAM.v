// Nets with names beginning with sys are in the system clock (sysClk)  domain.
// Other nets are in the ADC AXI clock (adcClk) domain.

module acquisitionBRAM #(
    parameter ACQUISITION_BUFFER_CAPACITY = -1,
    parameter AXI_CHANNEL_COUNT           = -1,
    parameter AXI_SAMPLE_WIDTH            = -1,
    parameter AXI_SAMPLES_PER_CLOCK       = -1,
    parameter DPRAM_DATA_WIDTH = AXI_CHANNEL_COUNT * AXI_SAMPLES_PER_CLOCK * AXI_SAMPLE_WIDTH
    ) (
    input              sysClk,
    input              sysCsrStrobe,
    input       [31:0] GPIO_OUT,
    output wire [31:0] sysStatus,

    input                        adcClk,
    input                        axiValid,
    input [DPRAM_DATA_WIDTH-1:0] axiData
    );

// Sanity check -- no $error in this version of Verilog
generate
if (ACQUISITION_BUFFER_CAPACITY % AXI_SAMPLES_PER_CLOCK ) begin
  ACQUISITION_BUFFER_CAPACITY_not_integer_multliple_of_AXI_SAMPLES_PER_CLOCK();
end
endgenerate

localparam DPRAM_ADDR_WIDTH = $clog2(ACQUISITION_BUFFER_CAPACITY / AXI_SAMPLES_PER_CLOCK);

reg [DPRAM_DATA_WIDTH-1:0] dpram [0:((1 << DPRAM_ADDR_WIDTH) - 1)], dpramQ;

///////////////////////////////////////////////////////////////////////////////
// ADC AXI Clock Domain
(*ASYNC_REG="true"*) reg acqEnable_m = 0;
reg sysAcqEnable = 0, acqEnable = 0;
reg [DPRAM_ADDR_WIDTH-1:0] acqAddress = 0;

always @(posedge adcClk) begin
    acqEnable_m <= sysAcqEnable;
    acqEnable   <= acqEnable_m;
    if (acqEnable && axiValid) begin
        dpram[acqAddress] <= axiData;
        acqAddress <= acqAddress + 1;
    end
end

///////////////////////////////////////////////////////////////////////////////
// System Clock Domain
localparam MUX_WIDTH = 16;
localparam MUX_SELECT_WIDTH = $clog2(AXI_CHANNEL_COUNT * AXI_SAMPLES_PER_CLOCK *
                                                  AXI_SAMPLE_WIDTH / MUX_WIDTH);
reg [DPRAM_ADDR_WIDTH-1:0] sysReadAddress = 0;
reg [MUX_SELECT_WIDTH-1:0] sysReadMuxSel;
reg [MUX_WIDTH-1:0] sysReadMuxQ;

always @(posedge sysClk) begin
    if (sysCsrStrobe) begin
        sysReadMuxSel <= GPIO_OUT[0+:MUX_SELECT_WIDTH];
        sysReadAddress <= GPIO_OUT[MUX_SELECT_WIDTH+:DPRAM_ADDR_WIDTH];
        sysAcqEnable <= GPIO_OUT[31];
    end
    dpramQ <= dpram[sysReadAddress];
    sysReadMuxQ <= dpramQ[sysReadMuxSel*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH];
end

assign sysStatus = { sysAcqEnable,
                     {32-1-DPRAM_ADDR_WIDTH-MUX_WIDTH{1'b0}},
                     acqAddress,
                     sysReadMuxQ };
endmodule
