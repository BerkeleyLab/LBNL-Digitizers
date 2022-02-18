// Control a Dynamic Reconfiguration Port
module drpControl #(
    parameter DRP_DATA_WIDTH = -1,
    parameter DRP_ADDR_WIDTH = -1,
    parameter DEBUG          = "false"
    ) (
    input              clk,
    (*mark_debug=DEBUG*)
    input              strobe,
    input       [31:0] dataIn,
    output wire [31:0] dataOut,
    output reg         resetControl = 0,

    (*mark_debug=DEBUG*) output reg drp_en,
    (*mark_debug=DEBUG*) output reg drp_we,
    (*mark_debug=DEBUG*) input wire drp_rdy,
    (*mark_debug=DEBUG*) output reg [DRP_ADDR_WIDTH-1:0] drp_addr,
    (*mark_debug=DEBUG*) output reg [DRP_DATA_WIDTH-1:0] drp_di,
    (*mark_debug=DEBUG*) input wire [DRP_DATA_WIDTH-1:0] drp_do);

(*mark_debug=DEBUG*) reg busy = 0, writing;
(*mark_debug=DEBUG*) reg [DRP_DATA_WIDTH-1:0] data;

assign dataOut = { busy,
                   {16-1-DRP_ADDR_WIDTH{1'b0}},
                   drp_addr,
                   data };
            
always @(posedge clk) begin
    if (strobe) begin
        if (dataIn[30]) begin
            resetControl <= dataIn[0];
        end
        else begin
            busy <= 1;
            drp_en <= 1;
            drp_we   <= dataIn[31];
            writing  <= dataIn[31];
            drp_addr <= dataIn[16+:DRP_ADDR_WIDTH];
            drp_di   <= dataIn[0+:DRP_DATA_WIDTH];
            data     <= dataIn[0+:DRP_DATA_WIDTH];
        end
    end
    else begin
        drp_en <= 0;
        drp_we <= 0;
        if (drp_rdy) begin
            busy <= 0;
            if (!writing) data <= drp_do;
        end
    end
end

endmodule

