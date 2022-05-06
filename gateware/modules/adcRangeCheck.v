// Monitor ADC extents

module adcRangeCheck #(
    parameter AXI_CHANNEL_COUNT           = -1,
    parameter AXI_SAMPLE_WIDTH            = -1,
    parameter AXI_SAMPLES_PER_CLOCK       = -1,
    parameter ADC_WIDTH                   = -1,
    parameter DATA_WIDTH = AXI_CHANNEL_COUNT * AXI_SAMPLES_PER_CLOCK * AXI_SAMPLE_WIDTH
    ) (
    input              sysClk,
    input              sysCsrStrobe,
    input       [31:0] GPIO_OUT,
    output wire [31:0] sysReadout,

    input                  adcClk,
    input [DATA_WIDTH-1:0] axiData
    );

reg sysLatchToggle = 0, sysShiftToggle;

////////////////////////////////////////////////////////////////////////////////
// ADC clock domain

// Acquisition/readout control
(*ASYNC_REG="true"*) reg adcLatchToggle_m = 0, adcShiftToggle_m = 0;
reg adcLatchToggle = 0, adcShiftToggle = 0;
reg adcLatchToggle_d = 0, adcShiftToggle_d = 0;
reg adcLatch = 0, adcShift = 0;

always @(posedge adcClk) begin
    adcLatchToggle_m <= sysLatchToggle;
    adcLatchToggle   <= adcLatchToggle_m;
    adcLatchToggle_d <= adcLatchToggle;
    adcLatch <= (adcLatchToggle != adcLatchToggle_d);

    adcShiftToggle_m <= sysShiftToggle;
    adcShiftToggle   <= adcShiftToggle_m;
    adcShiftToggle_d <= adcShiftToggle;
    adcShift <= (adcShiftToggle != adcShiftToggle_d);
end

// Find ranges
genvar i;
generate
for (i = 0 ; i < (AXI_CHANNEL_COUNT * AXI_SAMPLES_PER_CLOCK) ; i = i + 1) begin
                                                                        : sample
    wire signed [ADC_WIDTH-1:0] adcVal =
          axiData[(i*AXI_SAMPLE_WIDTH)+(AXI_SAMPLE_WIDTH-ADC_WIDTH)+:ADC_WIDTH];
    reg signed [ADC_WIDTH-1:0] min, max;
    always @(posedge adcClk) begin
        if (adcLatch || (adcVal < min)) min <= adcVal;
        if (adcLatch || (adcVal > max)) max <= adcVal;
    end
end
endgenerate

// Shuffle values into readout order
localparam SHIFTREG_WIDTH = 2*AXI_CHANNEL_COUNT*AXI_SAMPLES_PER_CLOCK*ADC_WIDTH;
wire [SHIFTREG_WIDTH-1:0] shiftIn;
generate
for (i = 0 ; i < (AXI_CHANNEL_COUNT * AXI_SAMPLES_PER_CLOCK) ; i = i + 1) begin
    assign shiftIn[((i*2)+0)*ADC_WIDTH+:ADC_WIDTH] = sample[i].min;
    assign shiftIn[((i*2)+1)*ADC_WIDTH+:ADC_WIDTH] = sample[i].max;
end
endgenerate

// Latch and read out
reg [SHIFTREG_WIDTH-1:0] shiftReg;
always @(posedge adcClk) begin
    if (adcLatch) begin
        shiftReg <= shiftIn;
    end
    else if (adcShift) begin
        shiftReg <= { {ADC_WIDTH{1'bx}},
                      shiftReg[ADC_WIDTH+:SHIFTREG_WIDTH-ADC_WIDTH] };
    end
end

////////////////////////////////////////////////////////////////////////////////
// System clock domain

// Request latch/shift operations, read from shift register
// No fancy clock crossing logic on result -- it's known to be stable
always @(posedge sysClk) begin
    if (sysCsrStrobe) begin
        if (GPIO_OUT[0]) sysLatchToggle <= !sysLatchToggle;
        if (GPIO_OUT[1]) sysShiftToggle <= !sysShiftToggle;
    end
end

generate
if (ADC_WIDTH <= 16) begin
    assign sysReadout = { 16'b0, shiftReg[ADC_WIDTH-1:0], {16-ADC_WIDTH{1'b0}} };
end
else begin
    assign sysReadout = { {32-ADC_WIDTH{shiftReg[ADC_WIDTH-1]}}, shiftReg[ADC_WIDTH-1:0]};
end
endgenerate

endmodule
