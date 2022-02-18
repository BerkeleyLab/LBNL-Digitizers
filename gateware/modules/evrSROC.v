// Monitor EVR and generate Storage Ring Orbit Clock
// Nets with names beginning with evr are in the EVR clock domain.

module evrSROC #(
    parameter SYSCLK_FREQUENCY = -1,
    parameter DEBUG            = "false"
    ) (
    input              sysClk,
    input              csrStrobe,
    input       [31:0] GPIO_OUT,
    output wire [31:0] csr,

    input                           evrClk,
    (*mark_debug=DEBUG*) input      evrHeartbeatMarker,
    (*mark_debug=DEBUG*) input      evrPulsePerSecondMarker,
    (*mark_debug=DEBUG*) output reg evrSROCsynced = 0,
    (*mark_debug=DEBUG*) output reg evrSROC = 0,
    (*mark_debug=DEBUG*) output reg evrSROCstrobe = 0);

localparam DIVISOR_WIDTH = 10;
localparam COUNTER_WIDTH = DIVISOR_WIDTH - 1;

reg [DIVISOR_WIDTH:0] sysClkDivisor = 0;
(*mark_debug=DEBUG*)reg [COUNTER_WIDTH-1:0] reloadLo, reloadHi;
always @(posedge sysClk) begin
    if (csrStrobe) begin
        sysClkDivisor <= GPIO_OUT[16+:DIVISOR_WIDTH];
    end
    reloadLo <= ((sysClkDivisor + 1) >> 1) - 1;
    reloadHi <= (sysClkDivisor >> 1) - 1;
end
wire heartBeatValid, pulsePerSecondValid;
assign csr = {{16-DIVISOR_WIDTH{1'b0}}, sysClkDivisor,
              {16-3{1'b0}}, pulsePerSecondValid, heartBeatValid, evrSROCsynced};

(*mark_debug=DEBUG*)reg [COUNTER_WIDTH-1:0] evrCounter = 0;
reg evrHeartbeatMarker_d;
always @(posedge evrClk) begin
    evrHeartbeatMarker_d <= evrHeartbeatMarker;
    if (evrHeartbeatMarker && !evrHeartbeatMarker_d) begin
        evrSROC <= 1;
        evrSROCstrobe <= 1;
        evrCounter <= reloadHi; 
        evrSROCsynced <= (!evrSROC && (evrCounter == 0));
    end
    else if (evrCounter == 0) begin
        evrSROC <= !evrSROC;
        if (evrSROC) begin
            evrSROCstrobe <= 0;
            evrCounter <= reloadLo;
        end
        else begin
            evrSROCstrobe <= 1;
            evrCounter <= reloadHi;
        end
    end else begin
        evrSROCstrobe <= 0;
        evrCounter <= evrCounter - 1;
    end
end

eventMarkerWatchdog #(.SYSCLK_FREQUENCY(SYSCLK_FREQUENCY),
                      .DEBUG(DEBUG))
  hbWatchdog (.sysClk(sysClk),
              .evrMarker(evrHeartbeatMarker),
              .isValid(heartBeatValid));

eventMarkerWatchdog #(.SYSCLK_FREQUENCY(SYSCLK_FREQUENCY),
                      .DEBUG(DEBUG))
  ppsWatchdog (.sysClk(sysClk),
               .evrMarker(evrPulsePerSecondMarker),
               .isValid(pulsePerSecondValid));
endmodule

module eventMarkerWatchdog #(
    parameter SYSCLK_FREQUENCY = 100000000,
    parameter DEBUG            = "false"
    ) (
    input      sysClk,
    input      evrMarker,
    output reg isValid = 0);

localparam UPPER_LIMIT = ((SYSCLK_FREQUENCY * 11) / 10);
localparam LOWER_LIMIT = ((SYSCLK_FREQUENCY *  9) / 10);
(*mark_debug=DEBUG*) reg [$clog2(LOWER_LIMIT+1)-1:0] watchdog;
(*mark_debug=DEBUG*) reg marker_m, marker, marker_d;

always @(posedge sysClk) begin
    marker_m <= evrMarker;
    marker   <= marker_m;
    marker_d <= marker;
    if (marker && !marker_d) begin
        watchdog <= 0;
        if ((watchdog > LOWER_LIMIT)
         && (watchdog < UPPER_LIMIT)) begin
            isValid <= 1;
        end
        else begin
            isValid <= 0;
        end
    end
    else if (watchdog < UPPER_LIMIT) begin
        watchdog <= watchdog + 1;
    end
    else begin
        isValid <= 0;
    end
end
endmodule
