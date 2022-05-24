module bpm_zcu208_top #(
    parameter ADC_WIDTH                 = 14,
    parameter AXI_SAMPLE_WIDTH          = ((ADC_WIDTH + 7) / 8) * 8,
    parameter SYSCLK_RATE               = 99999001,  // From block design
    parameter BD_ADC_CHANNEL_COUNT      = 16,
    parameter ADC_CHANNEL_DEBUG         = "false",
    parameter LO_WIDTH                  = 18,
    parameter MAG_WIDTH                 = 26,
    parameter PRODUCT_WIDTH             = AXI_SAMPLE_WIDTH + LO_WIDTH - 1,
    parameter ACQ_WIDTH                 = 32,
    parameter SITE_SAMPLES_PER_TURN     = 77,
    parameter SITE_CIC_FA_DECIMATE      = 76,
    parameter SITE_CIC_SA_DECIMATE      = 1000,
    parameter SITE_CIC_STAGES           = 2) (
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

    output  RFMC_DAC_00_P, RFMC_DAC_00_N,
    output  RFMC_DAC_01_P, RFMC_DAC_01_N,
    output  RFMC_DAC_02_P, RFMC_DAC_02_N,
    output  RFMC_DAC_03_P, RFMC_DAC_03_N,
    input   RF4_CLKO_B_C_P, RF4_CLKO_B_C_N,
    output  RFMC_DAC_04_P, RFMC_DAC_04_N,
    output  RFMC_DAC_05_P, RFMC_DAC_05_N,
    output  RFMC_DAC_06_P, RFMC_DAC_06_N,
    output  RFMC_DAC_07_P, RFMC_DAC_07_N,

    output wire SFP_REC_CLK_P,
    output wire SFP_REC_CLK_N,

    output wire EVR_FB_CLK,

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
    output wire       AFE_DACIO_00,

    output wire       CLK_SPI_MUX_SEL0,
    output wire       CLK_SPI_MUX_SEL1
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
wire sysClk, evrClk, adcClk, dacClk, prbsClk;
wire adcClkLocked, dacClkLocked;
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
OBUFDS OBUFDS_SFP_REC_CLK (
    .O(SFP_REC_CLK_P),
    .OB(SFP_REC_CLK_N),
    .I(evrClk)
);

`ifndef SIMULATE
// EVR FB clock forwarding for debug
wire EVR_FB_CLK_m;
ODDRE1 ODDRE1_EVR_FB_CLK (
   .Q(EVR_FB_CLK_m),
   .C(evrClk),
   .D1(1'b1),
   .D2(1'b0),
   .SR(1'b0)
);

OBUF #(
   .SLEW("FAST")
) OBUF_EVR_FB_CLK (
   .O(EVR_FB_CLK),
   .I(EVR_FB_CLK_m)
);
`endif

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
localparam SAMPLES_WIDTH    = CFG_AXI_SAMPLES_PER_CLOCK * AXI_SAMPLE_WIDTH;
localparam ACQ_SAMPLES_WIDTH = ACQ_WIDTH;
wire [(BD_ADC_CHANNEL_COUNT*SAMPLES_WIDTH)-1:0] adcsTDATA;
wire                 [BD_ADC_CHANNEL_COUNT-1:0] adcsTVALID;

// Calibration support
calibration #(
    .ADC_COUNT(CFG_ADC_CHANNEL_COUNT),
    .SAMPLES_PER_CLOCK(CFG_AXI_SAMPLES_PER_CLOCK),
    .ADC_WIDTH(ACQ_SAMPLES_WIDTH),
    .AXI_SAMPLE_WIDTH(ACQ_SAMPLES_WIDTH))
  calibration (
    .sysClk(sysClk),
    .csrStrobe(GPIO_STROBES[GPIO_IDX_CALIBRATION_CSR]),
    .GPIO_OUT(GPIO_OUT),
    .readout(GPIO_IN[GPIO_IDX_CALIBRATION_CSR]),
    .prbsClk(prbsClk),
    .trainingSignal(TRAINING_SIGNAL),
    .adcClk(adcClk),
    // This is wrong. acqTDATA contains data with different data
    // rates and valids
    .adcsTVALID(acqTVALID[0]),
    .adcsTDATA(acqTDATA[0+:CFG_ADC_CHANNEL_COUNT*ACQ_SAMPLES_WIDTH]));

/////////////////////////////////////////////////////////////////////////////
// Monitor range of signals at ADC inputs
adcRangeCheck #(
    .AXI_CHANNEL_COUNT(CFG_ADC_CHANNEL_COUNT),
    .AXI_SAMPLE_WIDTH(ACQ_SAMPLES_WIDTH),
    .AXI_SAMPLES_PER_CLOCK(CFG_AXI_SAMPLES_PER_CLOCK),
    .ADC_WIDTH(ACQ_SAMPLES_WIDTH))
  adcRangeCheck (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_ADC_RANGE_CSR]),
    .GPIO_OUT(GPIO_OUT),
    .sysReadout(GPIO_IN[GPIO_IDX_ADC_RANGE_CSR]),
    .adcClk(adcClk),
    // This is wrong. acqTDATA contains data with different data
    // rates and valids
    .axiValid(acqTVALID[0]),
    .axiData(acqTDATA[0+:CFG_ADC_CHANNEL_COUNT*ACQ_SAMPLES_WIDTH]));

/////////////////////////////////////////////////////////////////////////////
// Acquisition per style of firmware

`ifdef FIRMWARE_STYLE_BRAM
//
// Basic block-RAM acquisition for initial tests
//
acquisitionBRAM #(
    .ACQUISITION_BUFFER_CAPACITY(CFG_ACQUISITION_BUFFER_CAPACITY),
    .AXI_CHANNEL_COUNT(CFG_ADC_CHANNEL_COUNT),
    .AXI_SAMPLE_WIDTH(ACQ_SAMPLES_WIDTH),
    .AXI_SAMPLES_PER_CLOCK(CFG_AXI_SAMPLES_PER_CLOCK))
  acquisitionBRAM (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_ADC_0_CSR]),
    .GPIO_OUT(GPIO_OUT),
    .sysStatus(GPIO_IN[GPIO_IDX_ADC_0_CSR]),
    .adcClk(sysClk),
    .axiValid(acqTVALID),
    .axiData(acqTDATA));
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
    .AXI_SAMPLE_WIDTH(ACQ_SAMPLES_WIDTH),
    .ADC_WIDTH(ACQ_SAMPLES_WIDTH))
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
    .adcClk(sysClk),
    .axiValid(acqTVALID),
    .axiData(acqTDATA[0+:CFG_ADC_CHANNEL_COUNT*ACQ_SAMPLES_WIDTH]));

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
        .AXI_SAMPLE_WIDTH(ACQ_SAMPLES_WIDTH),
        .ADC_WIDTH(ACQ_SAMPLES_WIDTH),
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
        .sysData(GPIO_IN[GPIO_IDX_ADC_0_DATA+rOff]),
        .sysTriggerLocation(GPIO_IN[GPIO_IDX_ADC_0_TRIGGER_LOCATION+rOff]),
        .sysTriggerTimestamp({GPIO_IN[GPIO_IDX_ADC_0_SECONDS+rOff],
                              GPIO_IN[GPIO_IDX_ADC_0_TICKS+rOff]}),
        .evrClk(evrClk),
        .evrTimestamp(evrTimestamp),
        .adcClk(sysClk),
        .axiValid(acqTVALID[adc]),
        .axiData(acqTDATA[adc*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH]),
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
wire rfdc_dac0_clk;
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
assign GPIO_LEDS[7] = 1'b0;
assign GPIO_LEDS[6] = 1'b0;
assign GPIO_LEDS[5] = dacClkLocked;
assign GPIO_LEDS[4] = adcClkLocked;
assign GPIO_LEDS[3] = GPIO_IN[GPIO_IDX_SECONDS_SINCE_BOOT][1];
assign GPIO_LEDS[2] = GPIO_IN[GPIO_IDX_SECONDS_SINCE_BOOT][0];
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
    .user_sysref_adc(user_sysref_adc),

    // DAC tile 0 must be enabled dfor sysref
    // to be distributed to ADC/DAC for RFSoC GEN3
    .sysref_in_diff_n(SYSREF_RFSOC_C_N),
    .sysref_in_diff_p(SYSREF_RFSOC_C_P),

    .dacClk(dacClk),
    .dacClkLocked(dacClkLocked),
    .clk_dac0_0(rfdc_dac0_clk),

    // DAC tile 230 distributes clock to all others
    .dac45_clk_n(RF4_CLKO_B_C_N),
    .dac45_clk_p(RF4_CLKO_B_C_P),
    .user_sysref_dac(1'b0),

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

    .adc0stream_tdata(adcsTDATA[0*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc1stream_tdata(adcsTDATA[2*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc2stream_tdata(adcsTDATA[4*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc3stream_tdata(adcsTDATA[6*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc4stream_tdata(adcsTDATA[8*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc5stream_tdata(adcsTDATA[10*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc6stream_tdata(adcsTDATA[12*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc7stream_tdata(adcsTDATA[14*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc0Qstream_tdata(adcsTDATA[1*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc1Qstream_tdata(adcsTDATA[3*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc2Qstream_tdata(adcsTDATA[5*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc3Qstream_tdata(adcsTDATA[7*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc4Qstream_tdata(adcsTDATA[9*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc5Qstream_tdata(adcsTDATA[11*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc6Qstream_tdata(adcsTDATA[13*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc7Qstream_tdata(adcsTDATA[15*AXI_SAMPLE_WIDTH+:AXI_SAMPLE_WIDTH]),
    .adc0stream_tvalid(adcsTVALID[0]),
    .adc1stream_tvalid(adcsTVALID[2]),
    .adc2stream_tvalid(adcsTVALID[4]),
    .adc3stream_tvalid(adcsTVALID[6]),
    .adc4stream_tvalid(adcsTVALID[8]),
    .adc5stream_tvalid(adcsTVALID[10]),
    .adc6stream_tvalid(adcsTVALID[12]),
    .adc7stream_tvalid(adcsTVALID[14]),
    .adc0Qstream_tvalid(adcsTVALID[1]),
    .adc1Qstream_tvalid(adcsTVALID[3]),
    .adc2Qstream_tvalid(adcsTVALID[5]),
    .adc3Qstream_tvalid(adcsTVALID[7]),
    .adc4Qstream_tvalid(adcsTVALID[9]),
    .adc5Qstream_tvalid(adcsTVALID[11]),
    .adc6Qstream_tvalid(adcsTVALID[13]),
    .adc7Qstream_tvalid(adcsTVALID[15]),
    .adc0stream_tready(1'b1),
    .adc1stream_tready(1'b1),
    .adc2stream_tready(1'b1),
    .adc3stream_tready(1'b1),
    .adc4stream_tready(1'b1),
    .adc5stream_tready(1'b1),
    .adc6stream_tready(1'b1),
    .adc7stream_tready(1'b1),
    .adc0Qstream_tready(1'b1),
    .adc1Qstream_tready(1'b1),
    .adc2Qstream_tready(1'b1),
    .adc3Qstream_tready(1'b1),
    .adc4Qstream_tready(1'b1),
    .adc5Qstream_tready(1'b1),
    .adc6Qstream_tready(1'b1),
    .adc7Qstream_tready(1'b1),

    .vout0_v_n(RFMC_DAC_00_N),
    .vout0_v_p(RFMC_DAC_00_P),
    .vout1_v_n(RFMC_DAC_01_N),
    .vout1_v_p(RFMC_DAC_01_P),
    .vout2_v_n(RFMC_DAC_02_N),
    .vout2_v_p(RFMC_DAC_02_P),
    .vout3_v_n(RFMC_DAC_03_N),
    .vout3_v_p(RFMC_DAC_03_P),
    .vout4_v_n(RFMC_DAC_04_N),
    .vout4_v_p(RFMC_DAC_04_P),
    .vout5_v_n(RFMC_DAC_05_N),
    .vout5_v_p(RFMC_DAC_05_P),
    .vout6_v_n(RFMC_DAC_06_N),
    .vout6_v_p(RFMC_DAC_06_P),
    .vout7_v_n(RFMC_DAC_07_N),
    .vout7_v_p(RFMC_DAC_07_P)
    );

`endif // `ifndef SIMULATE

assign acqTVALID[0] = prelimProcRfCicFaMagValid;
assign acqTDATA[0*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH] = {
    {ACQ_SAMPLES_WIDTH-MAG_WIDTH{prelimProcRfCicFaMag0[MAG_WIDTH-1]}},
    prelimProcRfCicFaMag0
};

assign acqTVALID[1] = prelimProcRfCicFaMagValid;
assign acqTDATA[1*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH] = {
    {ACQ_SAMPLES_WIDTH-MAG_WIDTH{prelimProcRfCicFaMag1[MAG_WIDTH-1]}},
    prelimProcRfCicFaMag1
};

assign acqTVALID[2] = prelimProcRfTbtMagValid;
assign acqTDATA[2*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH] = {
    {ACQ_SAMPLES_WIDTH-MAG_WIDTH{prelimProcRfTbtMag0[MAG_WIDTH-1]}},
    prelimProcRfTbtMag0
};

assign acqTVALID[3] = prelimProcRfTbtMagValid;
assign acqTDATA[3*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH] = {
    {ACQ_SAMPLES_WIDTH-MAG_WIDTH{prelimProcRfTbtMag1[MAG_WIDTH-1]}},
    prelimProcRfTbtMag1
};

assign acqTVALID[4] = prelimProcRfFaMagValid;
assign acqTDATA[4*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH] = {
    {ACQ_SAMPLES_WIDTH-MAG_WIDTH{prelimProcRfFaMag0[MAG_WIDTH-1]}},
    prelimProcRfFaMag0
};

assign acqTVALID[5] = prelimProcRfFaMagValid;
assign acqTDATA[5*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH] = {
    {ACQ_SAMPLES_WIDTH-MAG_WIDTH{prelimProcRfFaMag1[MAG_WIDTH-1]}},
    prelimProcRfFaMag1
};

assign acqTVALID[6] = prelimProcSaValid;
assign acqTDATA[6*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH] = {
    {ACQ_SAMPLES_WIDTH-MAG_WIDTH{prelimProcRfMag0[MAG_WIDTH-1]}},
    prelimProcRfMag0
};

assign acqTVALID[7] = prelimProcSaValid;
assign acqTDATA[7*ACQ_SAMPLES_WIDTH+:ACQ_SAMPLES_WIDTH] = {
    {ACQ_SAMPLES_WIDTH-MAG_WIDTH{prelimProcRfMag1[MAG_WIDTH-1]}},
    prelimProcRfMag1
};

//
// Create slow (SA) and fast (FA) acquistion triggers
// based on event system trigger 0 (heartbeat).
//
wire evrFaMarker, evrSaMarker;
wire [31:0] sysFAstatus, sysSAstatus;
wire evrFaSynced, evrSaSynced;
acqSync acqSync(
    .sysClk(sysClk),
    .sysGPIO_OUT(GPIO_OUT),
    .sysFAstrobe(GPIO_STROBES[GPIO_IDX_EVR_FA_RELOAD]),
    .sysSAstrobe(GPIO_STROBES[GPIO_IDX_EVR_SA_RELOAD]),
    .sysFAstatus(sysFAstatus),
    .sysSAstatus(sysSAstatus),
    .evrClk(evrClk),
    .evrHeartbeat(evrHeartbeat),
    .evrFaMarker(evrFaMarker),
    .evrSaMarker(evrSaMarker));
assign GPIO_IN[GPIO_IDX_EVR_FA_RELOAD] = sysFAstatus;
assign GPIO_IN[GPIO_IDX_EVR_SA_RELOAD] = sysSAstatus;
assign evrFaSynced = sysFAstatus[31];
assign evrSaSynced = sysSAstatus[31];

//
// Preliminary processing (compute magnitude of ADC signals)
//
wire sysSingleTrig;
wire [32-MAG_WIDTH-1:0] magPAD = 0;
wire                 adcLoSynced, adcTbtLoadAccumulator, adcTbtLatchAccumulator;
wire                 adcMtLoadAndLatch;
wire                 prelimProcTbtToggle;
wire                 prelimProcRfTbtMagValid;
wire [MAG_WIDTH-1:0] prelimProcRfTbtMag0, prelimProcRfTbtMag1;
wire [MAG_WIDTH-1:0] prelimProcRfTbtMag2, prelimProcRfTbtMag3;
wire                 prelimProcFaToggle;
wire                 prelimProcRfFaMagValid;
wire [MAG_WIDTH-1:0] prelimProcRfFaMag0, prelimProcRfFaMag1;
wire [MAG_WIDTH-1:0] prelimProcRfFaMag2, prelimProcRfFaMag3;
wire                 prelimProcRfCicFaMagValid;
wire [MAG_WIDTH-1:0] prelimProcRfCicFaMag0, prelimProcRfCicFaMag1;
wire [MAG_WIDTH-1:0] prelimProcRfCicFaMag2, prelimProcRfCicFaMag3;
wire                 prelimProcSaToggle;
wire                 prelimProcSaValid;
wire [MAG_WIDTH-1:0] prelimProcRfMag0, prelimProcRfMag1;
wire [MAG_WIDTH-1:0] prelimProcRfMag2, prelimProcRfMag3;
wire [MAG_WIDTH-1:0] prelimProcPlMag0, prelimProcPlMag1;
wire [MAG_WIDTH-1:0] prelimProcPlMag2, prelimProcPlMag3;
wire [MAG_WIDTH-1:0] prelimProcPhMag0, prelimProcPhMag1;
wire [MAG_WIDTH-1:0] prelimProcPhMag2, prelimProcPhMag3;
wire [8*PRODUCT_WIDTH-1:0] rfProducts, plProducts, phProducts;
wire [LO_WIDTH-1:0] rfLOcos, rfLOsin;
wire [LO_WIDTH-1:0] plLOcos, plLOsin;
wire [LO_WIDTH-1:0] phLOcos, phLOsin;
wire [(BD_ADC_CHANNEL_COUNT*ACQ_SAMPLES_WIDTH)-1:0] acqTDATA;
wire [BD_ADC_CHANNEL_COUNT-1:0] acqTVALID;
wire prelimProcPtToggle, prelimProcOverflow;
wire [8*MAG_WIDTH-1:0] tbtSums;
wire tbtSumsValid;
wire [4*MAG_WIDTH-1:0] tbtMags;
wire tbtMagsValid;
assign GPIO_IN[GPIO_IDX_PRELIM_RF_MAG_0] = { magPAD, prelimProcRfMag0 };
assign GPIO_IN[GPIO_IDX_PRELIM_RF_MAG_1] = { magPAD, prelimProcRfMag1 };
assign GPIO_IN[GPIO_IDX_PRELIM_RF_MAG_2] = { magPAD, prelimProcRfMag2 };
assign GPIO_IN[GPIO_IDX_PRELIM_RF_MAG_3] = { magPAD, prelimProcRfMag3 };
assign GPIO_IN[GPIO_IDX_PRELIM_PT_LO_MAG_0] = { magPAD, prelimProcPlMag0 };
assign GPIO_IN[GPIO_IDX_PRELIM_PT_LO_MAG_1] = { magPAD, prelimProcPlMag1 };
assign GPIO_IN[GPIO_IDX_PRELIM_PT_LO_MAG_2] = { magPAD, prelimProcPlMag2 };
assign GPIO_IN[GPIO_IDX_PRELIM_PT_LO_MAG_3] = { magPAD, prelimProcPlMag3 };
assign GPIO_IN[GPIO_IDX_PRELIM_PT_HI_MAG_0] = { magPAD, prelimProcPhMag0 };
assign GPIO_IN[GPIO_IDX_PRELIM_PT_HI_MAG_1] = { magPAD, prelimProcPhMag1 };
assign GPIO_IN[GPIO_IDX_PRELIM_PT_HI_MAG_2] = { magPAD, prelimProcPhMag2 };
assign GPIO_IN[GPIO_IDX_PRELIM_PT_HI_MAG_3] = { magPAD, prelimProcPhMag3 };
preliminaryProcessing #(.SYSCLK_RATE(SYSCLK_RATE),
                        .ADC_WIDTH(AXI_SAMPLE_WIDTH),
                        .MAG_WIDTH(MAG_WIDTH),
                        .SAMPLES_PER_TURN(SITE_SAMPLES_PER_TURN),
                        .LO_WIDTH(LO_WIDTH),
                        .CIC_STAGES(SITE_CIC_STAGES),
                        .CIC_FA_DECIMATE(SITE_CIC_FA_DECIMATE),
                        .CIC_SA_DECIMATE(SITE_CIC_SA_DECIMATE),
                        .GPIO_LO_RF_ROW_CAPACITY(CFG_LO_RF_ROW_CAPACITY),
                        .GPIO_LO_PT_ROW_CAPACITY(CFG_LO_PT_ROW_CAPACITY))
  prelimProc(
    .clk(sysClk),
    .adcClk(adcClk),
    .adc0(adcsTDATA[0*SAMPLES_WIDTH+:SAMPLES_WIDTH]), // I0
    .adc1(adcsTDATA[1*SAMPLES_WIDTH+:SAMPLES_WIDTH]), // Q0
    .adc2(adcsTDATA[2*SAMPLES_WIDTH+:SAMPLES_WIDTH]), // I1
    .adc3(adcsTDATA[3*SAMPLES_WIDTH+:SAMPLES_WIDTH]), // Q1
    .adcExceedsThreshold(1'b0),
    .adcUseThisSample(1'b1),
    .evrClk(evrClk),
    .evrFaMarker(evrFaMarker),
    .evrSaMarker(evrSaMarker),
    .evrTimestamp(evrTimestamp),
    .evrPtTrigger(1'b0),
    .evrSinglePassTrigger(1'b0),
    .evrHbMarker(evrHeartbeat),
    .sysSingleTrig(sysSingleTrig),
    .sysTimestamp(evrTimestamp),
    .PT_P(),
    .PT_N(),
    .gpioData(GPIO_OUT),
    .localOscillatorAddressStrobe(GPIO_STROBES[GPIO_IDX_LOTABLE_ADDRESS]),
    .localOscillatorCsrStrobe(GPIO_STROBES[GPIO_IDX_LOTABLE_CSR]),
    .localOscillatorCsr(GPIO_IN[GPIO_IDX_LOTABLE_CSR]),
    .sumShiftCsrStrobe(GPIO_STROBES[GPIO_IDX_SUM_SHIFT_CSR]),
    .sumShiftCsr(GPIO_IN[GPIO_IDX_SUM_SHIFT_CSR]),
    .autotrimCsrStrobe(GPIO_STROBES[GPIO_IDX_AUTOTRIM_CSR]),
    .autotrimThresholdStrobe(GPIO_STROBES[GPIO_IDX_AUTOTRIM_THRESHOLD]),
    .autotrimGainStrobes({GPIO_STROBES[GPIO_IDX_ADC_GAIN_FACTOR_3],
                          GPIO_STROBES[GPIO_IDX_ADC_GAIN_FACTOR_2],
                          GPIO_STROBES[GPIO_IDX_ADC_GAIN_FACTOR_1],
                          GPIO_STROBES[GPIO_IDX_ADC_GAIN_FACTOR_0]}),
    .autotrimCsr(GPIO_IN[GPIO_IDX_AUTOTRIM_CSR]),
    .autotrimThreshold(GPIO_IN[GPIO_IDX_AUTOTRIM_THRESHOLD]),
    .gainRBK0(GPIO_IN[GPIO_IDX_ADC_GAIN_FACTOR_0]),
    .gainRBK1(GPIO_IN[GPIO_IDX_ADC_GAIN_FACTOR_1]),
    .gainRBK2(GPIO_IN[GPIO_IDX_ADC_GAIN_FACTOR_2]),
    .gainRBK3(GPIO_IN[GPIO_IDX_ADC_GAIN_FACTOR_3]),
    .adcLoSynced(adcLoSynced),
    .rfProductsDbg(rfProducts),
    .plProductsDbg(plProducts),
    .phProductsDbg(phProducts),
    .rfLOcosDbg(rfLOcos),
    .rfLOsinDbg(rfLOsin),
    .plLOcosDbg(plLOcos),
    .plLOsinDbg(plLOsin),
    .phLOcosDbg(phLOcos),
    .phLOsinDbg(phLOsin),
    .tbtSumsDbg(tbtSums),
    .tbtSumsValidDbg(tbtSumsValid),
    .tbtMagsDbg(tbtMags),
    .tbtMagsValidDbg(tbtMagsValid),
    .tbtToggle(),
    .rfTbtMagValid(prelimProcRfTbtMagValid),
    .rfTbtMag0(prelimProcRfTbtMag0),
    .rfTbtMag1(prelimProcRfTbtMag1),
    .rfTbtMag2(prelimProcRfTbtMag2),
    .rfTbtMag3(prelimProcRfTbtMag3),
    .cicFaMagValidDbg(prelimProcRfCicFaMagValid),
    .cicFaMag0Dbg(prelimProcRfCicFaMag0),
    .cicFaMag1Dbg(prelimProcRfCicFaMag1),
    .cicFaMag2Dbg(prelimProcRfCicFaMag2),
    .cicFaMag3Dbg(prelimProcRfCicFaMag3),
    .faToggle(prelimProcFaToggle),
    .adcTbtLoadAccumulator(adcTbtLoadAccumulator),
    .adcTbtLatchAccumulator(adcTbtLatchAccumulator),
    .adcMtLoadAndLatch(adcMtLoadAndLatch),
    .rfFaMagValid(prelimProcRfFaMagValid),
    .rfFaMag0(prelimProcRfFaMag0),
    .rfFaMag1(prelimProcRfFaMag1),
    .rfFaMag2(prelimProcRfFaMag2),
    .rfFaMag3(prelimProcRfFaMag3),
    .saToggle(prelimProcSaToggle),
    .saValid(prelimProcSaValid),
    .sysSaTimestamp({GPIO_IN[GPIO_IDX_SA_TIMESTAMP_SEC],
                     GPIO_IN[GPIO_IDX_SA_TIMESTAMP_TICKS]}),
    .rfMag0(prelimProcRfMag0),
    .rfMag1(prelimProcRfMag1),
    .rfMag2(prelimProcRfMag2),
    .rfMag3(prelimProcRfMag3),
    .plMag0(prelimProcPlMag0),
    .plMag1(prelimProcPlMag1),
    .plMag2(prelimProcPlMag2),
    .plMag3(prelimProcPlMag3),
    .phMag0(prelimProcPhMag0),
    .phMag1(prelimProcPhMag1),
    .phMag2(prelimProcPhMag2),
    .phMag3(prelimProcPhMag3),
    .ptToggle(prelimProcPtToggle),
    .overflowFlag(prelimProcOverflow)
);

evrLogger evrLogger (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_EVENT_LOG_CSR]),
    .sysGpioOut(GPIO_OUT),
    .sysCsr(GPIO_IN[GPIO_IDX_EVENT_LOG_CSR]),
    .sysDataTicks(GPIO_IN[GPIO_IDX_EVENT_LOG_TICKS]),
    .evrClk(evrClk),
    .evrChar(evrChars[7:0]),
    .evrCharIsK(evrCharIsK[0]));

wire [31:0] spiMuxSel;
gpioReg spiMuxSelReg (
    .sysClk(sysClk),
    .sysCsrStrobe(GPIO_STROBES[GPIO_IDX_CLK104_SPI_MUX_CSR]),
    .sysGpioOut(GPIO_OUT),
    .sysCsr(GPIO_IN[GPIO_IDX_CLK104_SPI_MUX_CSR]),
    .gpioOut(spiMuxSel));

assign CLK_SPI_MUX_SEL0 = spiMuxSel[0];
assign CLK_SPI_MUX_SEL1 = spiMuxSel[1];

endmodule
