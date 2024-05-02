// Wrapper around wizard-generated event receiver GTY block
//
// Provides word framing and access to dynamic reconfiguration port.
// Can't use manual or automatic bit slide to achieve framing since this
// leaves the recovered clock at a variable bit offset from the event
// generator reference clock.  Instead the transceiver is reset until it
// starts up with correct framing.  This has a 99.9% chance of happening
// within 135 attempts ((19/20)^135=0.983e-3).

module evrGTYwrapper #(
    parameter DEBUG = "false"
    ) (
    input              sysClk,
    input              csrStrobe,
    input              drpStrobe,
    input       [31:0] GPIO_OUT,
    output wire [31:0] csr,
    output wire [31:0] drp,

    input              refClk,
    input              RX_P, RX_N,
    output             TX_P, TX_N,
    output             evrTxClk,

    output wire                            evrClk,
    output wire                            evrRxSynchronized,
    (*mark_debug=DEBUG*) output reg [15:0] evrChars,
    (*mark_debug=DEBUG*) output reg  [1:0] evrCharIsK,
    (*mark_debug=DEBUG*) output reg  [1:0] evrCharIsComma);

localparam LOOPBACK = 3'd4; // 4 == Far end PMA loopback
localparam DRP_ADDR_WIDTH = 10;
localparam DRP_DATA_WIDTH = 16;

// Extract status bits of interest
wire [15:0] rxData;
wire [15:0] rxctrl0, rxctrl1;
wire  [7:0] rxctrl2, rxctrl3;
wire  [1:0] rxCharIsK        = rxctrl0[1:0];
wire  [1:0] rxDisparityError = rxctrl1[1:0];
wire  [1:0] rxCharIsComma    = rxctrl2[1:0];
wire  [1:0] rxCharNotInTable = rxctrl3[1:0];

//////////////////////////////////////////////////////////////////////////////
// Instantiate dynamic reconfiguration controller
wire                      drpen_in, drpwe_in, drprdy_out, eyescanReset;
wire [DRP_ADDR_WIDTH-1:0] drpaddr_in;
wire [DRP_DATA_WIDTH-1:0] drpdi_in, drpdo_out;
drpControl #(.DRP_DATA_WIDTH(DRP_DATA_WIDTH),
             .DRP_ADDR_WIDTH(DRP_ADDR_WIDTH),
             .DEBUG("false"))
  drpControl (
    .clk(sysClk),
    .strobe(drpStrobe),
    .dataIn(GPIO_OUT),
    .dataOut(drp),
    .resetControl(eyescanReset),
    .drp_en(drpen_in),
    .drp_we(drpwe_in),
    .drp_rdy(drprdy_out),
    .drp_addr(drpaddr_in),
    .drp_di(drpdi_in),
    .drp_do(drpdo_out));

//////////////////////////////////////////////////////////////////////////////
// Reset and bit slide control
reg reset_all = 0;
reg gtwiz_userclk_tx_reset = 0;
reg sysBuffBypassTxResetToggle = 0;
reg sysBuffBypassTxStartToggle = 0;
reg sysSlideRequestToggle = 0;
always @(posedge sysClk) begin
    if (csrStrobe) begin
        reset_all <= GPIO_OUT[0];
        gtwiz_userclk_tx_reset <= GPIO_OUT[1];
        if (GPIO_OUT[2]) begin
            sysBuffBypassTxResetToggle <= !sysBuffBypassTxResetToggle;
        end
        if (GPIO_OUT[3]) begin
            sysBuffBypassTxStartToggle <= !sysBuffBypassTxStartToggle;
        end
        if (GPIO_OUT[7]) begin
            sysSlideRequestToggle <= !sysSlideRequestToggle;
        end
    end
end

//////////////////////////////////////////////////////////////////////////////
// Transmitter buffer bypass
(*ASYNC_REG="true"*) reg buffBypassTxResetToggle_m = 0;
(*ASYNC_REG="true"*) reg buffBypassTxStartToggle_m = 0;
reg buffBypassTxResetToggle = 0, buffBypassTxResetToggle_d = 0;
reg buffBypassTxStartToggle = 0, buffBypassTxStartToggle_d = 0;
reg gtwiz_buffbypass_tx_reset = 0;
reg gtwiz_buffbypass_tx_start = 0;
reg [5:0] buffbypass_tx_reset_startDelay = 0;
wire buffbypass_tx_reset_startDelayDone = buffbypass_tx_reset_startDelay[5];
reg buffbypass_tx_reset_started = 0;
always @(posedge evrTxClk) begin
    buffBypassTxResetToggle_m <= sysBuffBypassTxResetToggle;
    buffBypassTxResetToggle   <= buffBypassTxResetToggle_m;
    buffBypassTxResetToggle_d <= buffBypassTxResetToggle;
    if (buffbypass_tx_reset_started) begin
        gtwiz_buffbypass_tx_reset <= (buffBypassTxResetToggle !=
                                                     buffBypassTxResetToggle_d);
    end
    else if (buffbypass_tx_reset_startDelayDone) begin
        gtwiz_buffbypass_tx_reset <= 1;
        buffbypass_tx_reset_started <= 1;
    end
    else begin
        buffbypass_tx_reset_startDelay <= buffbypass_tx_reset_startDelay + 1;
    end

    buffBypassTxStartToggle_m <= sysBuffBypassTxStartToggle;
    buffBypassTxStartToggle   <= buffBypassTxStartToggle_m;
    buffBypassTxStartToggle_d <= buffBypassTxStartToggle;
    gtwiz_buffbypass_tx_start <= (buffBypassTxStartToggle !=
                                                     buffBypassTxStartToggle_d);
end

//////////////////////////////////////////////////////////////////////////////
// Receiver alignment detection
localparam COMMAS_NEEDED = 30;
localparam COMMA_COUNTER_RELOAD = COMMAS_NEEDED - 1;
localparam COMMA_COUNTER_WIDTH = $clog2(COMMA_COUNTER_RELOAD+1) + 1;
reg [COMMA_COUNTER_WIDTH-1:0] commaCounter = COMMA_COUNTER_RELOAD;
assign evrRxSynchronized = commaCounter[COMMA_COUNTER_WIDTH-1];
// K character can only appear on word 0
wire rxDataErr = (rxCharNotInTable != 0) || rxCharIsK[1] || (rxDisparityError != 0);

(*ASYNC_REG="true"*) reg slideRequest_m = 0;
reg slideRequest_d0 = 0, slideRequest_d1 = 0;
reg rxslide = 0;
localparam FAULT_COUNTER_WIDTH = 10;
reg [FAULT_COUNTER_WIDTH-1:0] badCharCount = 0, badKcount = 0;
always @(posedge evrClk) begin
    slideRequest_m  <= sysSlideRequestToggle;
    slideRequest_d0 <= slideRequest_m;
    slideRequest_d1 <= slideRequest_d0;
    rxslide <= slideRequest_d0 ^ slideRequest_d1;

    if (evrRxSynchronized && !rxDataErr) begin
        evrChars <= rxData;
        evrCharIsK <= rxCharIsK[1:0];
        evrCharIsComma <= rxCharIsComma[1:0];
    end
    else begin
        evrChars <= 0;
        evrCharIsK <= 0;
        evrCharIsComma <= 0;
    end

    if (evrRxSynchronized) begin
        if (rxCharNotInTable != 0) badCharCount <= badCharCount + 1;
        if (rxCharIsK[1]) badKcount <= badKcount + 1;
    end

    if (rxDataErr) begin
        commaCounter <= COMMA_COUNTER_RELOAD;
    end
    else if (!evrRxSynchronized && rxCharIsK[0] && (rxData[7:0] == 8'hBC)) begin
        commaCounter <= commaCounter - 1;
    end
end

// Status register
wire reset_rx_done, reset_tx_done, cplllocked;
wire gtwiz_buffbypass_tx_done, gtwiz_buffbypass_tx_error;
assign csr = { badKcount, badCharCount,
               {32-FAULT_COUNTER_WIDTH-FAULT_COUNTER_WIDTH-8{1'b0}},
               evrRxSynchronized, reset_tx_done,
               reset_rx_done, cplllocked,
               gtwiz_buffbypass_tx_error, gtwiz_buffbypass_tx_done,
               gtwiz_userclk_tx_reset, reset_all };

//////////////////////////////////////////////////////////////////////////////
// Instantiate the transceiver
`ifndef SIMULATE
  evrGTY evrGTY (
    .gtwiz_userclk_tx_reset_in(gtwiz_userclk_tx_reset), // input wire [0 : 0] gtwiz_userclk_tx_reset_in
    .gtwiz_userclk_tx_srcclk_out(),              // output wire [0 : 0] gtwiz_userclk_tx_srcclk_out
    .gtwiz_userclk_tx_usrclk_out(),              // output wire [0 : 0] gtwiz_userclk_tx_usrclk_out
    .gtwiz_userclk_tx_usrclk2_out(evrTxClk),     // output wire [0 : 0] gtwiz_userclk_tx_usrclk2_out
    .gtwiz_userclk_tx_active_out(),              // output wire [0 : 0] gtwiz_userclk_tx_active_out
    .gtwiz_userclk_rx_reset_in(1'b0),            // input wire [0 : 0] gtwiz_userclk_rx_reset_in
    .gtwiz_userclk_rx_srcclk_out(),              // output wire [0 : 0] gtwiz_userclk_rx_srcclk_out
    .gtwiz_userclk_rx_usrclk_out(),              // output wire [0 : 0] gtwiz_userclk_rx_usrclk_out
    .gtwiz_userclk_rx_usrclk2_out(evrClk),       // output wire [0 : 0] gtwiz_userclk_rx_usrclk2_out
    .gtwiz_userclk_rx_active_out(),              // output wire [0 : 0] gtwiz_userclk_rx_active_out
  // .gtwiz_buffbypass_tx_reset_in(gtwiz_buffbypass_tx_reset), // input wire [0 : 0] gtwiz_buffbypass_tx_reset_in
  // .gtwiz_buffbypass_tx_start_user_in(gtwiz_buffbypass_tx_start), // input wire [0 : 0] gtwiz_buffbypass_tx_start_user_in
  // .gtwiz_buffbypass_tx_done_out(gtwiz_buffbypass_tx_done), // output wire [0 : 0] gtwiz_buffbypass_tx_done_out
  // .gtwiz_buffbypass_tx_error_out(gtwiz_buffbypass_tx_error), // output wire [0 : 0] gtwiz_buffbypass_tx_error_out
    .gtwiz_buffbypass_rx_reset_in(1'b0),         // input wire [0 : 0] gtwiz_buffbypass_rx_reset_in
    .gtwiz_buffbypass_rx_start_user_in(1'b0),    // input wire [0 : 0] gtwiz_buffbypass_rx_start_user_in
    .gtwiz_buffbypass_rx_done_out(),             // output wire [0 : 0] gtwiz_buffbypass_rx_done_out
    .gtwiz_buffbypass_rx_error_out(),            // output wire [0 : 0] gtwiz_buffbypass_rx_error_out
    .gtwiz_reset_clk_freerun_in(sysClk),         // input wire [0 : 0] gtwiz_reset_clk_freerun_in
    .gtwiz_reset_all_in(reset_all),              // input wire [0 : 0] gtwiz_reset_all_in
    .gtwiz_reset_tx_pll_and_datapath_in(1'b0),   // input wire [0 : 0] gtwiz_reset_tx_pll_and_datapath_in
    .gtwiz_reset_tx_datapath_in(1'b0),           // input wire [0 : 0] gtwiz_reset_tx_datapath_in
    .gtwiz_reset_rx_pll_and_datapath_in(1'b0),   // input wire [0 : 0] gtwiz_reset_rx_pll_and_datapath_in
    .gtwiz_reset_rx_datapath_in(1'b0),           // input wire [0 : 0] gtwiz_reset_rx_datapath_in
    .gtwiz_reset_rx_cdr_stable_out(),            // output wire [0 : 0] gtwiz_reset_rx_cdr_stable_out
    .gtwiz_reset_tx_done_out(reset_tx_done),     // output wire [0 : 0] gtwiz_reset_tx_done_out
    .gtwiz_reset_rx_done_out(reset_rx_done),     // output wire [0 : 0] gtwiz_reset_rx_done_out
    .gtwiz_userdata_tx_in(16'h00BC),             // input wire [15 : 0] gtwiz_userdata_tx_in
    .gtwiz_userdata_rx_out(rxData),              // output wire [15 : 0] gtwiz_userdata_rx_out
    .cplllockdetclk_in(sysClk),                  // input wire [0 : 0] cplllockdetclk_in
    .cplllocken_in(1'b1),                        // input wire [0 : 0] cplllocken_in
    .cpllreset_in(1'b0),                         // input wire [0 : 0] cpllreset_in
    .drpaddr_in(drpaddr_in),                     // input wire [9 : 0] drpaddr_in
    .drpclk_in(sysClk),                          // input wire [0 : 0] drpclk_in
    .drpdi_in(drpdi_in),                         // input wire [15 : 0] drpdi_in
    .drpen_in(drpen_in),                         // input wire [0 : 0] drpen_in
    .drpwe_in(drpwe_in),                         // input wire [0 : 0] drpwe_in
    .eyescanreset_in(eyescanReset),              // input wire [0 : 0] eyescanreset_in
    .gtrefclk0_in(refClk),                       // input wire [0 : 0] gtrefclk0_in
    .gtyrxn_in(RX_N),                            // input wire [0 : 0] gtyrxn_in
    .gtyrxp_in(RX_P),                            // input wire [0 : 0] gtyrxp_in
    .loopback_in(LOOPBACK),                      // input wire [2 : 0] loopback_in
    .rx8b10ben_in(1'b1),                         // input wire [0 : 0] rx8b10ben_in
    .rxcommadeten_in(1'b1),                      // input wire [0 : 0] rxcommadeten_in
    .rxmcommaalignen_in(1'b0),                   // input wire [0 : 0] rxmcommaalignen_in
    .rxpcommaalignen_in(1'b0),                   // input wire [0 : 0] rxpcommaalignen_in
    .rxslide_in(rxslide),                        // input wire [0 : 0] rxslide_in
    .tx8b10ben_in(1'b1),                         // input wire [0 : 0] tx8b10ben_in
    .txctrl0_in(16'h0000),                       // input wire [15 : 0] txctrl0_in
    .txctrl1_in(16'h0000),                       // input wire [15 : 0] txctrl1_in
    .txctrl2_in(8'h01),                          // input wire [7 : 0] txctrl2_in
    .cplllock_out(cplllocked),                   // output wire [0 : 0] cplllock_out
    .drpdo_out(drpdo_out),                       // output wire [15 : 0] drpdo_out
    .drprdy_out(drprdy_out),                     // output wire [0 : 0] drprdy_out
    .gtpowergood_out(),                          // output wire [0 : 0] gtpowergood_out
    .gtytxn_out(TX_N),                           // output wire [0 : 0] gtytxn_out
    .gtytxp_out(TX_P),                           // output wire [0 : 0] gtytxp_out
    .rxbyteisaligned_out(),                      // output wire [0 : 0] rxbyteisaligned_out
    .rxbyterealign_out(),                        // output wire [0 : 0] rxbyterealign_out
    .rxcommadet_out(),                           // output wire [0 : 0] rxcommadet_out
    .rxctrl0_out(rxctrl0),                       // output wire [15 : 0] rxctrl0_out
    .rxctrl1_out(rxctrl1),                       // output wire [15 : 0] rxctrl1_out
    .rxctrl2_out(rxctrl2),                       // output wire [7 : 0] rxctrl2_out
    .rxctrl3_out(rxctrl3),                       // output wire [7 : 0] rxctrl3_out
    .rxpmaresetdone_out(),                       // output wire [0 : 0] rxpmaresetdone_out
    .txpmaresetdone_out(),                       // output wire [0 : 0] txpmaresetdone_out
    .txprgdivresetdone_out()                     // output wire [0 : 0] txprgdivresetdone_out
  );
`endif

endmodule
