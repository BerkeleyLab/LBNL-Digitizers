// Log event arrival
module evrLogger #(
    parameter ADDR_WIDTH = 10
    ) (
    input  wire        sysClk,
    input  wire        sysCsrStrobe,
    input  wire [31:0] sysGpioOut,
    output wire [31:0] sysCsr,
    output wire [31:0] sysDataTicks,
    output wire [31:0] sysSeconds,
    output wire [31:0] sysFraction,

    input  wire        evrClk,
    input  wire  [7:0] evrChar,
    input  wire        evrCharIsK,

    input        [31:0] evrSeconds,
    input        [31:0] evrFraction

);

reg [ADDR_WIDTH-1:0] sysReadAddress = 0, evrWriteAddress = 0;
reg sysRunning = 0;
wire [3:0] addrWidth = ADDR_WIDTH;
reg [32+32+8+32-1:0] dpram[0:(1<<ADDR_WIDTH)-1], dpramQ;
assign sysSeconds = dpramQ[103:72];
assign sysFraction = dpramQ[71:40];
wire [7:0] sysDataEvent = dpramQ[39:32];
assign sysDataTicks = dpramQ[31:0];
assign sysCsr = { sysRunning, {3{1'b0}}, addrWidth,
                  sysDataEvent,
                  {16-ADDR_WIDTH{1'b0}}, evrWriteAddress };
always @(posedge sysClk) begin
    if (sysCsrStrobe) begin
        sysReadAddress <= sysGpioOut[ADDR_WIDTH-1:0];
        sysRunning <= sysGpioOut[31];
    end
    dpramQ <= dpram[sysReadAddress];
end

(*ASYNC_REG="true"*) reg evrRunning_m;
reg        evrRunning;
reg        evrDPRAMwen;
reg  [7:0] evrDPRAMevent;
reg [31:0] evrTickCounter = 0;
always @(posedge evrClk) begin
    evrRunning_m <= sysRunning;
    evrRunning   <= evrRunning_m;
    evrTickCounter <= evrTickCounter + 1;
    evrDPRAMwen <= evrRunning && !evrCharIsK && (evrChar != 0);
    evrDPRAMevent <= evrChar;
    if (evrDPRAMwen) begin
        dpram[evrWriteAddress] <= { evrSeconds, evrFraction, evrDPRAMevent, evrTickCounter};
        evrWriteAddress <= evrWriteAddress + 1;
    end
    else if (!evrRunning) begin
        evrWriteAddress <= 0;
    end
end

endmodule
