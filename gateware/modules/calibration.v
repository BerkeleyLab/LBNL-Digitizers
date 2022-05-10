// Measure mean value of a single ADC channel.
// Produce training signal
// Net names beginning with 'adc' are in ADC clock domain.

module calibration #(
    parameter ADC_COUNT         = -1,
    parameter ADC_WIDTH         = -1,
    parameter SAMPLES_PER_CLOCK = -1,
    parameter AXI_SAMPLE_WIDTH  = -1
    ) (
    input              sysClk,
    input              csrStrobe,
    input       [31:0] GPIO_OUT,
    output wire [31:0] readout,

    output wire        prbsClk,
    output wire        trainingSignal,

    input                                                    adcClk,
    input                                                    adcsTVALID,
    input [ADC_COUNT*SAMPLES_PER_CLOCK*AXI_SAMPLE_WIDTH-1:0] adcsTDATA);

// Ensure that we take enough samples for the RF ADC Gain Calibration
// Block and Time Skew Calibration Block to settle.
localparam SAMPLECOUNT  = (1 << 23);
localparam RESULT_WIDTH = 16;
localparam MUXSEL_WIDTH = $clog2(ADC_COUNT);

///////////////////////////////////////////////////////////////////////////////
// System clock domain

// From ADC clock domain, but used in system clock domain.
// Mean value will be stable when read.
wire adcDone;
wire [RESULT_WIDTH-1:0] adcMeanValue;

// Delay start request by one cycle to give the ADC clock domain
// firmware time to select the correct ADC channel.
reg csrToggle = 0, csrToggle_d = 0;
reg trainingSignalOn = 0;
reg [MUXSEL_WIDTH-1:0] sysSelect = 0;
(* ASYNC_REG = "true" *) reg sysAdcDone_m = 0;
reg sysAdcDone = 0, sysAdcDone_d = 0;
reg sysBusy = 0;
always @(posedge sysClk) begin
    sysAdcDone_m <= adcDone;
    if (csrStrobe) begin
        sysSelect <= GPIO_OUT[24+:MUXSEL_WIDTH];
        trainingSignalOn <= GPIO_OUT[30];
        if (GPIO_OUT[31]) begin
            csrToggle <= !csrToggle;
            sysBusy <= 1;
        end
    end
    else begin
        sysAdcDone   <= sysAdcDone_m;
        sysAdcDone_d <= sysAdcDone;
        if (sysAdcDone && !sysAdcDone_d) begin
            sysBusy <= 0;
        end
    end
    csrToggle_d <= csrToggle;
end

assign readout = { sysBusy, trainingSignalOn, {2{1'b0}},
                   {4-MUXSEL_WIDTH{1'b0}}, sysSelect,
                   {24-RESULT_WIDTH{1'b0}}, adcMeanValue };

// Calibration signal PRBS generator
localparam PRBS_SHIFT_WIDTH = 15;
localparam PRBS_SHIFT_TAP   = 13;

(* ASYNC_REG = "true" *) reg prbsEnable_m = 0;
reg prbsEnable = 0;
reg [PRBS_SHIFT_WIDTH-1:0] prbsShift = 0;
wire prbsNext = !(prbsShift[PRBS_SHIFT_WIDTH-1] ^ prbsShift[PRBS_SHIFT_TAP]);

prbsMMCM prbsMMCM (
    .clk_out1(prbsClk),
    .clk_in1(sysClk));
always @(posedge prbsClk) begin
    prbsEnable_m <= trainingSignalOn;
    prbsEnable   <= prbsEnable_m;
    if (prbsEnable) begin
        prbsShift <= { prbsShift[PRBS_SHIFT_WIDTH-2:0], prbsNext };
    end
    else begin
        prbsShift <= 0;
    end
end
assign trainingSignal = prbsShift[0];

///////////////////////////////////////////////////////////////////////////////
// ADC clock domain

genvar i;

//
// Select the channel of interest
//
reg signed [ADC_WIDTH-1:0] adcMux[0:SAMPLES_PER_CLOCK-1];
reg                        adcMuxValid;
generate
for (i = 0 ; i < SAMPLES_PER_CLOCK ; i = i + 1) begin : muxSel
    always @(posedge adcClk) begin
        adcMuxValid <= adcsTVALID;
        adcMux[i] <= adcsTDATA[(((sysSelect*SAMPLES_PER_CLOCK) + (i+1)) *
                                                AXI_SAMPLE_WIDTH)-1-:ADC_WIDTH];
    end
end

//
// Sum pairs of channels
//
localparam PAIR_SUM_WIDTH = ADC_WIDTH + 1;
reg                             adcPairSumValid;
reg signed [PAIR_SUM_WIDTH-1:0] adcPairSum[0:SAMPLES_PER_CLOCK/2-1];

for (i = 0 ; i < SAMPLES_PER_CLOCK / 2 ; i = i + 1) begin : pairSum
    always @(posedge adcClk) begin
        adcPairSumValid <= adcMuxValid;
        adcPairSum[i] <= adcMux[2*i+0] + adcMux[2*i+1];
    end
end

//
// Sum the pair sums into a single per-clock sum
//
localparam PER_CLOCK_SUM_WIDTH = ADC_WIDTH + $clog2(SAMPLES_PER_CLOCK);
wire                                  adcPartialSumValid;
wire signed [PER_CLOCK_SUM_WIDTH-1:0] adcPartialSum[0:SAMPLES_PER_CLOCK/2-1];
reg                                   adcSumValid;
reg  signed [PER_CLOCK_SUM_WIDTH-1:0] adcSum;

assign adcPartialSumValid = adcPairSumValid;
assign adcPartialSum[0] = adcPairSum[0];
for (i = 1 ; i < SAMPLES_PER_CLOCK / 2 ; i = i + 1) begin : sum
    assign adcPartialSum[i] = adcPartialSum[i-1] + adcPairSum[i];
end
endgenerate

//
// Accumulate per-clock sums on demand
//
localparam ACCUMULATOR_WIDTH = ADC_WIDTH + $clog2(SAMPLECOUNT);
localparam SUM_COUNTER_WIDTH = $clog2(SAMPLECOUNT/SAMPLES_PER_CLOCK) + 1;

reg signed [ACCUMULATOR_WIDTH-1:0] adcAccumulator;
assign adcMeanValue = adcAccumulator[ACCUMULATOR_WIDTH-1-:RESULT_WIDTH];
reg [SUM_COUNTER_WIDTH-1:0] adcSumCounter = ~0;
assign adcDone = adcSumCounter[SUM_COUNTER_WIDTH-1];

(* ASYNC_REG = "true" *) reg adcCSRtoggle_m = 0;
reg adcCSRtoggle = 0, adcCSRtoggle_d = 0, adcStart = 0;

always @(posedge adcClk) begin
    adcCSRtoggle_m <= csrToggle_d;
    adcCSRtoggle   <= adcCSRtoggle_m;
    adcCSRtoggle_d <= adcCSRtoggle;
    adcStart <= adcCSRtoggle ^ adcCSRtoggle_d;
    if (adcStart) begin
        adcSumCounter <= 0;
        adcAccumulator <= 0;
    end
    else if (!adcDone && adcSumValid) begin
        adcSumCounter <= adcSumCounter + 1;
        adcAccumulator <= adcAccumulator + adcSum;
    end
    adcSumValid <= adcPartialSumValid;
    adcSum <= adcPartialSum[SAMPLES_PER_CLOCK/2-1];
end

endmodule
