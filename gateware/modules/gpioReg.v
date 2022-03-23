// Trivial GPIO reg for SPI MUX selection
module gpioReg (
    input  wire        sysClk,
    input  wire        sysCsrStrobe,
    input  wire [31:0] sysGpioOut,
    output wire [31:0] sysCsr,

    output wire [31:0] gpioOut);

reg [31:0] gpioOutReg = 0;
always @(posedge sysClk) begin
    if (sysCsrStrobe) begin
        gpioOutReg <= sysGpioOut;
    end
end

assign sysCsr = gpioOutReg;
assign gpioOut = gpioOutReg;

endmodule
