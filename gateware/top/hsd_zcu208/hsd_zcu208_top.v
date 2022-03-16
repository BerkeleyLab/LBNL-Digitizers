module hsd_zcu208_top #(
    parameter ADC_WIDTH            = 12,
    parameter SYSCLK_RATE          = 100000000,  // From block design
    parameter BD_ADC_CHANNEL_COUNT = 8,
    parameter ADC_CHANNEL_DEBUG    = "false"
    ) (
    input  USER_MGT_SI570_CLK_P, USER_MGT_SI570_CLK_N,
    input  SFP2_RX_P, SFP2_RX_N,
    output SFP2_TX_P, SFP2_TX_N,
    output SFP2_TX_ENABLE,

    input  FPGA_REFCLK_OUT_C_P, FPGA_REFCLK_OUT_C_N,
    input  SYSREF_FPGA_C_P, SYSREF_FPGA_C_N,
    input  SYSREF_RFSOC_C_P, SYSREF_RFSOC_C_N,
    input  RFMC_ADC_00_P, RFMC_ADC_00_N,
    input  RFMC_ADC_01_P, RFMC_ADC_01_N,
    input  RF1_CLKO_B_C_P, RF1_CLKO_B_C_N,
    input  RFMC_ADC_02_P, RFMC_ADC_02_N,
    input  RFMC_ADC_03_P, RFMC_ADC_03_N,
    input  RFMC_ADC_04_P, RFMC_ADC_04_N,
    input  RFMC_ADC_05_P, RFMC_ADC_05_N,
    input  RFMC_ADC_06_P, RFMC_ADC_06_N,
    input  RFMC_ADC_07_P, RFMC_ADC_07_N,

    output wire SFP_REC_CLK_P,
    output wire SFP_REC_CLK_N,

    input             GPIO_SW_W,
    input             GPIO_SW_E,
    input             GPIO_SW_N,
    input       [7:0] DIP_SWITCH,
    output wire [7:0] GPIO_LEDS,

    output wire [7:0] AFE_SPI_CSB,
    output wire       AFE_SPI_SDI,
    input             AFE_SPI_SDO,
    output wire       AFE_SPI_CLK,
    output wire       TRAINING_SIGNAL,
    output wire       BCM_SROC_GND,
    output wire       AFE_DACIO_00
);

`include "firmwareBuildDate.v"

//////////////////////////////////////////////////////////////////////////////
// Static outputs
assign SFP2_TX_ENABLE = 1'b1;

//////////////////////////////////////////////////////////////////////////////
// General-purpose I/O block
// Include file is machine generated from C header
`include "gpioIDX.v"
wire                    [31:0] GPIO_IN[0:GPIO_IDX_COUNT-1];
wire                    [31:0] GPIO_OUT;
wire      [GPIO_IDX_COUNT-1:0] GPIO_STROBES;
wire [(GPIO_IDX_COUNT*32)-1:0] GPIO_IN_FLATTENED;
genvar i;
generate
for (i = 0 ; i < GPIO_IDX_COUNT ; i = i + 1) begin : gpio_flatten
    assign GPIO_IN_FLATTENED[i*32+:32] = GPIO_IN[i];
end
endgenerate
assign GPIO_IN[GPIO_IDX_FIRMWARE_BUILD_DATE] = FIRMWARE_BUILD_DATE;

//////////////////////////////////////////////////////////////////////////////
// Clocks
wire sysClk, evrClk, adcClk, prbsClk;
wire adcClkLocked;
wire sysReset_n;

// Get USER MGT reference clock
// Configure ODIV2 to run at O/2.
wire USER_MGT_SI570_CLK, USER_MGT_SI570_CLK_O2;
IBUFDS_GTE4 #(.REFCLK_HROW_CK_SEL(2'b01))
  EVR_GTY_refclkBuf(.I(USER_MGT_SI570_CLK_P),
                              .IB(USER_MGT_SI570_CLK_N),
                              .CEB(1'b0),
                              .O(USER_MGT_SI570_CLK),
                              .ODIV2(USER_MGT_SI570_CLK_O2));
wire mgtRefClkMonitor;
BUFG_GT userMgtChkClkBuf (.O(mgtRefClkMonitor),
                          .CE(1'b1),
                          .CEMASK(1'b0),
                          .CLR(1'b0),
                          .CLRMASK(1'b0),
                          .DIV(3'd0),
                          .I(USER_MGT_SI570_CLK_O2));

//////////////////////////////////////////////////////////////////////////////
// Front panel controls
// Also provide on-board alternatives in case the front panel board is absent.
(*ASYNC_REG="TRUE"*) reg Reset_RecoveryModeSwitch_m, Reset_RecoveryModeSwitch;
(*ASYNC_REG="TRUE"*) reg DisplayModeSwitch_m, DisplayModeSwitch;
always @(posedge sysClk) begin
    Reset_RecoveryModeSwitch_m <= GPIO_SW_W;
    DisplayModeSwitch_m        <= GPIO_SW_E;
    Reset_RecoveryModeSwitch   <= Reset_RecoveryModeSwitch_m;
    DisplayModeSwitch          <= DisplayModeSwitch_m;
end

//////////////////////////////////////////////////////////////////////////////
// Timekeeping
clkIntervalCounters #(.CLK_RATE(SYSCLK_RATE))
  clkIntervalCounters (
    .clk(sysClk),
    .microsecondsSinceBoot(GPIO_IN[GPIO_IDX_MICROSECONDS_SINCE_BOOT]),
    .secondsSinceBoot(GPIO_IN[GPIO_IDX_SECONDS_SINCE_BOOT]));

/////////////////////////////////////////////////////////////////////////////
// Event receiver support
wire        evrRxSynchronized;
wire [15:0] evrChars;
wire  [1:0] evrCharIsK;
wire  [1:0] evrCharIsComma;
wire [63:0] evrTimestamp;

wire evrTxClk;
evrGTYwrapper #(.DEBUG("false"))
  evrGTYwrapper (
    .sysClk(sysClk),
    .csrStrobe(GPIO_STROBES[GPIO_IDX_GTY_CSR]),
    .drpStrobe(GPIO_STROBES[GPIO_IDX_EVR_GTY_DRP]),
    .GPIO_OUT(GPIO_OUT),
    .csr(GPIO_IN[GPIO_IDX_GTY_CSR]),
    .drp(GPIO_IN[GPIO_IDX_EVR_GTY_DRP]),
    .refClk(USER_MGT_SI570_CLK),
    .evrTxClk(evrTxClk),
    .RX_N(SFP2_RX_N),
    .RX_P(SFP2_RX_P),
    .TX_N(SFP2_TX_N),
    .TX_P(SFP2_TX_P),
    .evrClk(evrClk),
    .evrRxSynchronized(evrRxSynchronized),
    .evrChars(evrChars),
    .evrCharIsK(evrCharIsK),
    .evrCharIsComma(evrCharIsComma));

// EVR triggers
wire [7:0] evrTriggerBus;
wire evrHeartbeat = evrTriggerBus[0];
wire evrPulsePerSecond = evrTriggerBus[1];
wire evrSROCsynced;
assign GPIO_LEDS[0] = evrHeartbeat;
assign GPIO_LEDS[1] = evrPulsePerSecond;

// Reference clock for RF ADC jitter cleaner
// EVR clock ~124.91 MHz
OBUFDS OBUFDS_SFP_REC_CLK (
    .O(SFP_REC_CLK_P),
    .OB(SFP_REC_CLK_N),
    .I(evrClk)
);

// Check EVR markers
wire [31:0] evrSyncStatus;
evrSROC #(.SYSCLK_FREQUENCY(SYSCLK_RATE),
          .DEBUG("false"))
  evrSROC(.sysClk(sysClk),
          .csrStrobe(GPIO_STROBES[GPIO_IDX_EVR_SYNC_CSR]),
          .GPIO_OUT(GPIO_OUT),
          .csr(evrSyncStatus),
          .evrClk(evrClk),
          .evrHeartbeatMarker(evrHeartbeat),
          .evrPulsePerSecondMarker(evrPulsePerSecond),
          .evrSROCsynced(evrSROCsynced),
          .evrSROC(AFE_DACIO_00),
          .evrSROCstrobe());
assign GPIO_IN[GPIO_IDX_EVR_SYNC_CSR] = evrSyncStatus;
wire isPPSvalid = evrSyncStatus[2];

/////////////////////////////////////////////////////////////////////////////
// Generate tile synchronization user_sysref_adc
wire FPGA_REFCLK_OUT_C;
wire FPGA_REFCLK_OUT_C_unbuf;
wire user_sysref_adc;

IBUFDS FPGA_REFCLK_IBUFDS(
    .I(FPGA_REFCLK_OUT_C_P),
    .IB(FPGA_REFCLK_OUT_C_N),
    .O(FPGA_REFCLK_OUT_C_unbuf)
);
BUFG FPGA_REFCLK_BUFG(
    .I(FPGA_REFCLK_OUT_C_unbuf),
    .O(FPGA_REFCLK_OUT_C)
);

wire SYSREF_FPGA_C_unbuf;
IBUFDS SYSREF_FPGA_IBUFDS(
    .I(SYSREF_FPGA_C_P),
    .IB(SYSREF_FPGA_C_N),
    .O(SYSREF_FPGA_C_unbuf)
);

sysrefSync #(.DEBUG("false"))
  sysrefSync (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_SYSREF_CSR]),
    .GPIO_OUT(GPIO_OUT),
    .sysStatusReg(GPIO_IN[GPIO_IDX_SYSREF_CSR]),
    .FPGA_REFCLK_OUT_C(FPGA_REFCLK_OUT_C),
    .SYSREF_FPGA_C_UNBUF(SYSREF_FPGA_C_unbuf),
    .adcClk(adcClk),
    .user_sysref_adc(user_sysref_adc));

/////////////////////////////////////////////////////////////////////////////
// Triggers
localparam ACQ_TRIGGER_BUS_WIDTH = 7;
reg [ACQ_TRIGGER_BUS_WIDTH-1:0] adcEventTriggerStrobes = 0;
// Software trigger
reg [3:0] sysSoftTriggerCounter = 0;
wire sysSoftTrigger = sysSoftTriggerCounter[3];
always @(posedge sysClk) begin
    if (GPIO_STROBES[GPIO_IDX_SOFT_TRIGGER]) begin
        sysSoftTriggerCounter = ~0;
    end
    else if (sysSoftTrigger) begin
        sysSoftTriggerCounter <= sysSoftTriggerCounter - 1;
    end
end

// Get event and soft triggers into ADC clock domain
(*ASYNC_REG="TRUE"*) reg [ACQ_TRIGGER_BUS_WIDTH-1:0] adcEventTriggers_m = 0;
(*ASYNC_REG="TRUE"*) reg [ACQ_TRIGGER_BUS_WIDTH-1:0] adcEventTriggers = 0;
reg [ACQ_TRIGGER_BUS_WIDTH-1:0] adcEventTriggers_d = 0;
always @(posedge adcClk) begin
    adcEventTriggers_m <= { evrTriggerBus[7:2], sysSoftTrigger };
    adcEventTriggers   <= adcEventTriggers_m;
    adcEventTriggers_d <= adcEventTriggers;
    adcEventTriggerStrobes <= (adcEventTriggers & ~adcEventTriggers_d);
end

/////////////////////////////////////////////////////////////////////////////
// Acquisition common
localparam AXI_SAMPLE_WIDTH = ((ADC_WIDTH + 7) / 8) * 8;
localparam SAMPLES_WIDTH    = CFG_AXI_SAMPLES_PER_CLOCK * AXI_SAMPLE_WIDTH;
wire [(BD_ADC_CHANNEL_COUNT*SAMPLES_WIDTH)-1:0] adcsTDATA;
wire                 [BD_ADC_CHANNEL_COUNT-1:0] adcsTVALID;

// Calibration support
calibration #(
    .ADC_COUNT(CFG_ADC_CHANNEL_COUNT),
    .SAMPLES_PER_CLOCK(CFG_AXI_SAMPLES_PER_CLOCK),
    .ADC_WIDTH(ADC_WIDTH),
    .AXI_SAMPLE_WIDTH(AXI_SAMPLE_WIDTH))
  calibration (
    .sysClk(sysClk),
    .csrStrobe(GPIO_STROBES[GPIO_IDX_CALIBRATION_CSR]),
    .GPIO_OUT(GPIO_OUT),
    .readout(GPIO_IN[GPIO_IDX_CALIBRATION_CSR]),
    .prbsClk(prbsClk),
    .trainingSignal(TRAINING_SIGNAL),
    .adcClk(adcClk),
    .adcsTDATA(adcsTDATA[0+:CFG_ADC_CHANNEL_COUNT*SAMPLES_WIDTH]));

/////////////////////////////////////////////////////////////////////////////
// Monitor range of signals at ADC inputs
adcRangeCheck #(
    .AXI_CHANNEL_COUNT(CFG_ADC_CHANNEL_COUNT),
    .AXI_SAMPLE_WIDTH(AXI_SAMPLE_WIDTH),
    .AXI_SAMPLES_PER_CLOCK(CFG_AXI_SAMPLES_PER_CLOCK),
    .ADC_WIDTH(ADC_WIDTH))
  adcRangeCheck (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_ADC_RANGE_CSR]),
    .GPIO_OUT(GPIO_OUT),
    .sysReadout(GPIO_IN[GPIO_IDX_ADC_RANGE_CSR]),
    .adcClk(adcClk),
    .axiData(adcsTDATA[0+:CFG_ADC_CHANNEL_COUNT*SAMPLES_WIDTH]));

/////////////////////////////////////////////////////////////////////////////
// Acquisition per style of firmware

`ifdef FIRMWARE_STYLE_BRAM
//
// Basic block-RAM acquisition for initial tests
//
acquisitionBRAM #(
    .ACQUISITION_BUFFER_CAPACITY(CFG_ACQUISITION_BUFFER_CAPACITY),
    .AXI_CHANNEL_COUNT(CFG_ADC_CHANNEL_COUNT),
    .AXI_SAMPLE_WIDTH(AXI_SAMPLE_WIDTH),
    .AXI_SAMPLES_PER_CLOCK(CFG_AXI_SAMPLES_PER_CLOCK))
  acquisitionBRAM (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_ADC_0_CSR]),
    .GPIO_OUT(GPIO_OUT),
    .sysStatus(GPIO_IN[GPIO_IDX_ADC_0_CSR]),
    .adcClk(adcClk),
    .axiData(adcsTDATA));
`endif

`ifdef FIRMWARE_STYLE_BCM
//
// Bunch current monitor acquisition
//
acquisitionBCM #(
    .CHANNEL_COUNT(CFG_ADC_CHANNEL_COUNT),
    .SAMPLE_CAPACITY(CFG_ACQUISITION_BUFFER_CAPACITY),
    .MAX_PASSES_PER_ACQUISITION(CFG_MAX_PASSES_PER_ACQUISITION),
    .AXI_SAMPLES_PER_CLOCK(CFG_AXI_SAMPLES_PER_CLOCK),
    .AXI_SAMPLE_WIDTH(AXI_SAMPLE_WIDTH),
    .ADC_WIDTH(ADC_WIDTH))
  acquisitionBCM (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_ACQUISITION_CSR]),
    .sysAddrStrobe(GPIO_STROBES[GPIO_IDX_ACQUISITION_READOUT]),
    .GPIO_OUT(GPIO_OUT),
    .sysStatusReg(GPIO_IN[GPIO_IDX_ACQUISITION_CSR]),
    .sysReadoutReg(GPIO_IN[GPIO_IDX_ACQUISITION_READOUT]),
    .triggerTimestamp({GPIO_IN[GPIO_IDX_ACQUISITION_SECONDS],
                       GPIO_IN[GPIO_IDX_ACQUISITION_TICKS]}),
    .evrPostCycleTrigger(evrTriggerBus[2]),
    .evrClk(evrClk),
    .evrInjectionTrigger(evrTriggerBus[3]),
    .evrTimestamp(evrTimestamp),
    .adcClk(adcClk),
    .axiData(adcsTDATA[0+:CFG_ADC_CHANNEL_COUNT*SAMPLES_WIDTH]));

assign BCM_SROC_GND = 0;
`endif

`ifdef FIRMWARE_STYLE_HSD
//
// High speed digitizer acquisition
//
localparam NUMBER_OF_BONDED_GROUPS =
                    (CFG_ADC_CHANNEL_COUNT + CFG_ADCS_PER_BONDED_GROUP -1 ) /
                                                      CFG_ADCS_PER_BONDED_GROUP;
genvar adc;
//generate
for (i = 0 ; i < NUMBER_OF_BONDED_GROUPS ; i = i + 1) begin
 wire bondedWriteEnable[0:CFG_ADCS_PER_BONDED_GROUP-1];
 wire [$clog2(CFG_ACQUISITION_BUFFER_CAPACITY/CFG_AXI_SAMPLES_PER_CLOCK)-1:0]
                              bondedWriteAddress[0:CFG_ADCS_PER_BONDED_GROUP-1];
 for (adc = i * CFG_ADCS_PER_BONDED_GROUP ;
                             (adc < ((i + 1) * CFG_ADCS_PER_BONDED_GROUP)) &&
                             (adc < CFG_ADC_CHANNEL_COUNT); adc = adc + 1) begin
    localparam integer rOff = adc * GPIO_IDX_PER_ADC;
    acquisitionHSD #(
        .ACQUISITION_BUFFER_CAPACITY(CFG_ACQUISITION_BUFFER_CAPACITY),
        .LONG_SEGMENT_CAPACITY(CFG_LONG_SEGMENT_CAPACITY),
        .SHORT_SEGMENT_CAPACITY(CFG_SHORT_SEGMENT_CAPACITY),
        .EARLY_SEGMENTS_COUNT(CFG_EARLY_SEGMENTS_COUNT),
        .SEGMENT_PRETRIGGER_COUNT(CFG_SEGMENT_PRETRIGGER_COUNT),
        .AXI_SAMPLES_PER_CLOCK(CFG_AXI_SAMPLES_PER_CLOCK),
        .AXI_SAMPLE_WIDTH(AXI_SAMPLE_WIDTH),
        .ADC_WIDTH(ADC_WIDTH),
        .TRIGGER_BUS_WIDTH(ACQ_TRIGGER_BUS_WIDTH),
        .DEBUG((adc == 0) ? ADC_CHANNEL_DEBUG : "false"))
      adcChannel (
        .sysClk(sysClk),
        .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_ADC_0_CSR+rOff]),
        .sysTriggerConfigStrobe(GPIO_STROBES[GPIO_IDX_ADC_0_TRIGGER_CONFIG+rOff]),
        .sysAcqConfig1Strobe(GPIO_STROBES[GPIO_IDX_ADC_0_CONFIG_1+rOff]),
        .sysAcqConfig2Strobe(GPIO_STROBES[GPIO_IDX_ADC_0_CONFIG_2+rOff]),
        .GPIO_OUT(GPIO_OUT),
        .sysStatus(GPIO_IN[GPIO_IDX_ADC_0_CSR+rOff]),
        .sysTriggerLocation(GPIO_IN[GPIO_IDX_ADC_0_TRIGGER_LOCATION+rOff]),
        .sysTriggerTimestamp({GPIO_IN[GPIO_IDX_ADC_0_SECONDS+rOff],
                              GPIO_IN[GPIO_IDX_ADC_0_TICKS+rOff]}),
        .evrClk(evrClk),
        .evrTimestamp(evrTimestamp),
        .adcClk(adcClk),
        .axiData(adcsTDATA[adc*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
        .eventTriggerStrobes(adcEventTriggerStrobes),
        .bondedWriteEnableIn(bondedWriteEnable[0]),
        .bondedWriteAddressIn(bondedWriteAddress[0]),
        .bondedWriteEnableOut(bondedWriteEnable[adc%CFG_ADCS_PER_BONDED_GROUP]),
        .bondedWriteAddressOut(bondedWriteAddress[adc%CFG_ADCS_PER_BONDED_GROUP]));
 end
end
`endif

/////////////////////////////////////////////////////////////////////////////
// Measure clock rates
reg   [2:0] frequencyMonitorSelect;
wire [29:0] measuredFrequency;
always @(posedge sysClk) begin
    if (GPIO_STROBES[GPIO_IDX_FREQ_MONITOR_CSR]) begin
        frequencyMonitorSelect <= GPIO_OUT[2:0];
    end
end
assign GPIO_IN[GPIO_IDX_FREQ_MONITOR_CSR] = { 2'b0, measuredFrequency };
wire rfdc_adc0_clk;
freq_multi_count #(
        .NF(8),  // number of frequency counters in a block
        .NG(1),  // number of frequency counter blocks
        .gw(4),  // Gray counter width
        .cw(1),  // macro-cycle counter width
        .rw($clog2(SYSCLK_RATE*4/3)), // reference counter width
        .uw(30)) // unknown counter width
  frequencyCounters (
    .unk_clk({mgtRefClkMonitor, prbsClk,
              FPGA_REFCLK_OUT_C, rfdc_adc0_clk,
              adcClk, evrTxClk,
              evrClk, sysClk}),
    .refclk(sysClk),
    .refMarker(isPPSvalid & evrPulsePerSecond),
    .addr(frequencyMonitorSelect),
    .frequency(measuredFrequency));

/////////////////////////////////////////////////////////////////////////////
// Miscellaneous
assign GPIO_IN[GPIO_IDX_USER_GPIO_CSR] = {
               Reset_RecoveryModeSwitch, DisplayModeSwitch, 5'b0, adcClkLocked,
               evrTriggerBus,
               8'b0,
               DIP_SWITCH }; // DFE Serial Number

//////////////////////////////////////////////////////////////////////////////
// Analog front end SPI components
afeSPI #(.CLK_RATE(SYSCLK_RATE),
         .CSB_WIDTH(8),
         .BIT_RATE(12500000),
         .DEBUG("false"))
  afeSPI (
    .clk(sysClk),
    .csrStrobe(GPIO_STROBES[GPIO_IDX_AFE_SPI_CSR]),
    .gpioOut(GPIO_OUT),
    .status(GPIO_IN[GPIO_IDX_AFE_SPI_CSR]),
    .SPI_CLK(AFE_SPI_CLK),
    .SPI_CSB(AFE_SPI_CSB),
    .SPI_SDI(AFE_SPI_SDI),
    .SPI_SDO(AFE_SPI_SDO));

//////////////////////////////////////////////////////////////////////////////
// Interlocks
reg interlockRelayControl = 0;
wire interlockResetButton = GPIO_SW_N;
wire interlockRelayOpen = 1'b0;
wire interlockRelayClosed = 1'b1;
assign GPIO_LEDS[7] = interlockResetButton;
assign GPIO_LEDS[6] = interlockRelayOpen;
assign GPIO_LEDS[5] = interlockRelayClosed;
assign GPIO_LEDS[4] = interlockRelayControl;
assign GPIO_LEDS[3:2] = 0;
always @(posedge sysClk) begin
    if (GPIO_STROBES[GPIO_IDX_INTERLOCK_CSR]) begin
        interlockRelayControl <= GPIO_OUT[0];
    end
end
assign GPIO_IN[GPIO_IDX_INTERLOCK_CSR] = { 28'b0, interlockRelayClosed,
                                                  interlockRelayOpen,
                                                  1'b0,
                                                  interlockResetButton };

// Make this a black box for simulation
`ifndef SIMULATE
//////////////////////////////////////////////////////////////////////////////
// ZYNQ processor system
system
  system_i (
    .sysClk(sysClk),
    .sysReset_n(sysReset_n),

    .GPIO_IN(GPIO_IN_FLATTENED),
    .GPIO_OUT(GPIO_OUT),
    .GPIO_STROBES(GPIO_STROBES),

    .evrCharIsComma(evrCharIsComma),
    .evrCharIsK(evrCharIsK),
    .evrClk(evrClk),
    .evrChars(evrChars),
    .evrMgtResetDone(evrRxSynchronized),
    .evrTriggerBus(evrTriggerBus),
    .evrTimestamp(evrTimestamp),

    .FPGA_REFCLK_OUT_C(FPGA_REFCLK_OUT_C),
    .adcClk(adcClk),
    .adcClkLocked(adcClkLocked),
    .clk_adc0_0(rfdc_adc0_clk),

    // ADC tile 225 distributes clock to all others
    //.adc01_clk_n(),
    //.adc01_clk_p(),
    .adc23_clk_n(RF1_CLKO_B_C_N),
    .adc23_clk_p(RF1_CLKO_B_C_P),
    //.adc45_clk_n(),
    //.adc45_clk_p(),
    //.adc67_clk_n(),
    //.adc67_clk_p(),
    .sysref_in_diff_n(SYSREF_RFSOC_C_N),
    .sysref_in_diff_p(SYSREF_RFSOC_C_P),
    .user_sysref_adc(user_sysref_adc),

    .vin0_v_n(RFMC_ADC_00_N),
    .vin0_v_p(RFMC_ADC_00_P),
    .vin1_v_n(RFMC_ADC_01_N),
    .vin1_v_p(RFMC_ADC_01_P),
    .vin2_v_n(RFMC_ADC_02_N),
    .vin2_v_p(RFMC_ADC_02_P),
    .vin3_v_n(RFMC_ADC_03_N),
    .vin3_v_p(RFMC_ADC_03_P),
    .vin4_v_n(RFMC_ADC_04_N),
    .vin4_v_p(RFMC_ADC_04_P),
    .vin5_v_n(RFMC_ADC_05_N),
    .vin5_v_p(RFMC_ADC_05_P),
    .vin6_v_n(RFMC_ADC_06_N),
    .vin6_v_p(RFMC_ADC_06_P),
    .vin7_v_n(RFMC_ADC_07_N),
    .vin7_v_p(RFMC_ADC_07_P),

    .adc0stream_tdata(adcsTDATA[0*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
    .adc1stream_tdata(adcsTDATA[1*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
    .adc2stream_tdata(adcsTDATA[2*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
    .adc3stream_tdata(adcsTDATA[3*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
    .adc4stream_tdata(adcsTDATA[4*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
    .adc5stream_tdata(adcsTDATA[5*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
    .adc6stream_tdata(adcsTDATA[6*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
    .adc7stream_tdata(adcsTDATA[7*SAMPLES_WIDTH+:SAMPLES_WIDTH]),
    .adc0stream_tvalid(adcsTVALID[0]),
    .adc1stream_tvalid(adcsTVALID[1]),
    .adc2stream_tvalid(adcsTVALID[2]),
    .adc3stream_tvalid(adcsTVALID[3]),
    .adc4stream_tvalid(adcsTVALID[4]),
    .adc5stream_tvalid(adcsTVALID[5]),
    .adc6stream_tvalid(adcsTVALID[6]),
    .adc7stream_tvalid(adcsTVALID[7]),
    .adc0stream_tready(1'b1),
    .adc1stream_tready(1'b1),
    .adc2stream_tready(1'b1),
    .adc3stream_tready(1'b1),
    .adc4stream_tready(1'b1),
    .adc5stream_tready(1'b1),
    .adc6stream_tready(1'b1),
    .adc7stream_tready(1'b1)
    );

`endif // `ifndef SIMULATE


evrLogger evrLogger (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_EVENT_LOG_CSR]),
    .sysGpioOut(GPIO_OUT),
    .sysCsr(GPIO_IN[GPIO_IDX_EVENT_LOG_CSR]),
    .sysDataTicks(GPIO_IN[GPIO_IDX_EVENT_LOG_TICKS]),
    .evrClk(evrClk),
    .evrChar(evrChars[7:0]),
    .evrCharIsK(evrCharIsK[0]));


endmodule
