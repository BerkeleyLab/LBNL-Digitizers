// Scale time by a factor of 1000

`timescale 1us / 1ns

module todReceiver_tb #(
    parameter [7:0] EVCODE_SHIFT_ZERO     = 8'h70,
    parameter [7:0] EVCODE_SHIFT_ONE      = 8'h71,
    parameter [7:0] EVCODE_SECONDS_MARKER = 8'h7D,
    parameter NOMINAL_CLK_RATE = 100_000,
    parameter TIMESTAMP_WIDTH = 64,
    parameter TOD_SECONDS_WIDTH = TIMESTAMP_WIDTH / 2,
    parameter TOD_FRACTION_WIDTH = TIMESTAMP_WIDTH / 2
);

reg clk;

integer cc;
integer errors = 0;

initial begin
    if ($test$plusargs("vcd")) begin
        $dumpfile("todReceiver.vcd");
        $dumpvars(5,todReceiver_tb);
    end

    clk = 1;
    for (cc = 0; cc < 500000; cc = cc+1) begin
        clk = 1; #5;
        clk = 0; #5;
    end

    if (errors == 0)
        $display("PASS");
end

//////////////////////////////////
// PPS generation
//////////////////////////////////

integer tBase = 0;
reg pps = 0;

always @(posedge clk) begin
    if (($time - tBase) > 1000000) begin
        if (($time - tBase) > 1000100) begin
            tBase = tBase + 1000000;
        end
        pps <= ~$time & 1;
    end
    else begin
        pps <= 0;
    end
end

reg pps_d = 0;
reg ppsStrobe = 0;

always @(posedge clk) begin
    pps_d <= pps;

    ppsStrobe <= 0;
    if (pps && !pps_d) begin
        ppsStrobe <= 1;
    end
end

//////////////////////////////////
// DUT
//////////////////////////////////

reg [7:0] evCode = 0;
reg evCodeValid = 0;
wire [TIMESTAMP_WIDTH-1:0] timestamp;
wire timestampValid;
todReceiver #(
    .NOMINAL_CLK_RATE(NOMINAL_CLK_RATE),
    .TIMESTAMP_WIDTH(TIMESTAMP_WIDTH)
) DUT (
    .clk(clk),
    .rst(1'b0),

    .evCode(evCode),
    .evCodeValid(evCodeValid),

    .tooManyBitsCounter(),
    .tooFewBitsCounter(),
    .outOfSeqCounter(),
    .timestamp(timestamp),
    .timestampValid(timestampValid)
);

//////////////////////////////////
// Stimulus
//////////////////////////////////
localparam TOD_DELAY = 32;
localparam TOD_DELAY_WIDTH = $clog2(TOD_DELAY+1) + 1;
localparam TOD_BIT_COUNTER_WIDTH = $clog2(TOD_SECONDS_WIDTH+1) + 1;

reg todStart = 0;
reg [TOD_SECONDS_WIDTH-1:0] seconds = 32'h12345677;
reg [TOD_SECONDS_WIDTH-1:0] todShiftReg = 0;
reg [TOD_DELAY_WIDTH-1:0] todDelay = TOD_DELAY - 1;
reg [TOD_BIT_COUNTER_WIDTH-1:0] todBitCounter = TOD_SECONDS_WIDTH - 1;
wire todDelayDone = todDelay[TOD_DELAY_WIDTH-1];
wire todBitCounterDone = todBitCounter[TOD_BIT_COUNTER_WIDTH-1];

reg ppsRequest = 0;
reg todRequest = 0;

always @(posedge clk) begin
    if (ppsStrobe) begin
        ppsRequest <= 1;
        todDelay <= TOD_DELAY - 1;
        todBitCounter <= TOD_SECONDS_WIDTH - 1;
        todStart <= 1;
    end
    else if (todDelayDone) begin
        if (!todBitCounterDone && !todRequest) begin
            todRequest <= 1;
            todBitCounter <= todBitCounter - 1;

            if (todStart) begin
                todStart <= 0;
                todShiftReg <= seconds;
            end
            else begin
                todShiftReg <= {todShiftReg[TOD_SECONDS_WIDTH-2:0], 1'bx};
            end
        end
    end
    else begin
        todDelay <= todDelay + 1;
    end
end

always @(posedge clk) begin
    evCode <= 0;
    evCodeValid <= 0;

    if (ppsRequest) begin
        ppsRequest <= 0;

        evCode <= EVCODE_SECONDS_MARKER;
        evCodeValid <= 1;
    end
    else if (todRequest) begin
        todRequest <= 0;

        evCode <= todShiftReg[TOD_SECONDS_WIDTH-1] ? EVCODE_SHIFT_ONE :
            EVCODE_SHIFT_ZERO;
        evCodeValid <= 1;
    end
end

endmodule
