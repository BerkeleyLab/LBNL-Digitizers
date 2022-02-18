/////////////////////////////////////////////////////////////////////////////
// Generate tile synchronization user_sysref_adc
// As shown in PG269 (v2.1) Zynq UltraScale+ RFSoC RF Data Converter
//   Figure 105: Clocking for Multi-Tile Synchronization
//   Figures 81 and 83 display similar examples
// Two stage sampling:
//   First stage at FPGA_REFCLK_OUT_C rate
//   Second stage at ADC AXI rate (adcClk)
// Check that SYSREF capture is free of race conditions.
// Output FPGA_REFCLK_OUT_C so it can be taken to an MMCM block
// which multiplies it to provide the ADC AXI clock (adcClk).

module sysrefSync #(
    parameter COUNTER_WIDTH = 8,
    parameter DEBUG         = "false"
    ) (
    input              sysClk,
    input              sysCsrStrobe,
    input       [31:0] GPIO_OUT,
    output wire [31:0] sysStatusReg,

    input              FPGA_REFCLK_OUT_C_P, FPGA_REFCLK_OUT_C_N,
    output             FPGA_REFCLK_OUT_C,
    input              SYSREF_FPGA_C_P, SYSREF_FPGA_C_N,

    input              adcClk,
    output reg         user_sysref_adc);

wire FPGA_REFCLK_OUT_C_s;
IBUFDS FPGA_REFCLK_IBUFDS(.I(FPGA_REFCLK_OUT_C_P),
                          .IB(FPGA_REFCLK_OUT_C_N),
                          .O(FPGA_REFCLK_OUT_C_s));
BUFG FPGA_REFCLK_BUFG(.I(FPGA_REFCLK_OUT_C_s), .O(FPGA_REFCLK_OUT_C));

wire sysrefRaw;
(*ASYNC_REG="TRUE"*) reg sysrefSampled;
IBUFDS sysrefBuf(.I(SYSREF_FPGA_C_P), .IB(SYSREF_FPGA_C_N), .O(sysrefRaw));

// Domain-crossing values
(*mark_debug=DEBUG*) reg adcClkNewValueToggle = 0;
(*mark_debug=DEBUG*) reg [COUNTER_WIDTH-1:0] adcClkCount = 0;
(*mark_debug=DEBUG*) reg refClkNewValueToggle = 0;
(*mark_debug=DEBUG*) reg [COUNTER_WIDTH-1:0] refClkCount = 0;

//////////////////////////////////////////////////////////////////////////////
// System clock domain
// Verify that number of clocks between sysref assertions meet expectations.

(*ASYNC_REG="TRUE"*) reg newAdcValueToggle_m = 0, newAdcValueToggle = 0;
reg newAdcValueMatch = 0;
(*ASYNC_REG="TRUE"*) reg newRefValueToggle_m = 0, newRefValueToggle = 0;
reg newRefValueMatch = 0;

(*mark_debug=DEBUG*) reg [COUNTER_WIDTH-1:0] expectedAdcCount, expectedRefCount;
(*mark_debug=DEBUG*) reg adcFault = 0, refFault = 0;

assign sysStatusReg = { adcFault, {16-1-COUNTER_WIDTH{1'b0}}, adcClkCount,
                        refFault, {16-1-COUNTER_WIDTH{1'b0}}, refClkCount };

always @(posedge sysClk) begin
    newAdcValueToggle_m <= adcClkNewValueToggle;
    newRefValueToggle_m <= refClkNewValueToggle;
    if (sysCsrStrobe) begin
        if (GPIO_OUT[31]) adcFault <= 0;
        if (GPIO_OUT[15]) refFault <= 0;
        if (!GPIO_OUT[31] && !GPIO_OUT[15]) begin
            expectedRefCount <= GPIO_OUT[0+:COUNTER_WIDTH];
            expectedAdcCount <= GPIO_OUT[16+:COUNTER_WIDTH];
        end
    end
    else begin
        newAdcValueToggle <= newAdcValueToggle_m;
        if (newAdcValueToggle != newAdcValueMatch) begin
            newAdcValueMatch <= !newAdcValueMatch;
            if (adcClkCount != expectedAdcCount) begin
                adcFault <= 1;
            end
        end
        newRefValueToggle <= newRefValueToggle_m;
        if (newRefValueToggle != newRefValueMatch) begin
            newRefValueMatch <= !newRefValueMatch;
            if (refClkCount != expectedRefCount) begin
                refFault <= 1;
            end
        end
    end
end

//////////////////////////////////////////////////////////////////////////////
// FPGA_REFCLK_OUT_C clock domain
// Count clocks between SYSREF assertions.

(*mark_debug=DEBUG*) reg [COUNTER_WIDTH-1:0] refClkCounter = 0;
reg sysrefSampled_d = 1;

always @(posedge FPGA_REFCLK_OUT_C) begin
    sysrefSampled   <= sysrefRaw;
    sysrefSampled_d <= sysrefSampled;
    if (sysrefSampled && !sysrefSampled_d) begin
        refClkCount <= refClkCounter;
        refClkCounter <= 0;
        refClkNewValueToggle <= !refClkNewValueToggle;
    end
    else begin
        refClkCounter <= refClkCounter + 1;
    end
end


//////////////////////////////////////////////////////////////////////////////
// ADC clock domain
// Count clocks between SYSREF assertions.

(*mark_debug=DEBUG*) reg [COUNTER_WIDTH-1:0] adcClkCounter = 0;
reg user_sysref_adc_d = 1;

always @(posedge adcClk) begin
    user_sysref_adc   <= sysrefSampled;
    user_sysref_adc_d <= user_sysref_adc;
    if (user_sysref_adc && !user_sysref_adc_d) begin
        adcClkCount <= adcClkCounter;
        adcClkCounter <= 0;
        adcClkNewValueToggle <= !adcClkNewValueToggle;
    end
    else begin
        adcClkCounter <= adcClkCounter + 1;
    end
end

endmodule
