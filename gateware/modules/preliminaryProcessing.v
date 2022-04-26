// Preliminary signal processing
// Convert ADC values to synchronously-demodulated or RMS magnitudes
//
module preliminaryProcessing #(
    parameter SYSCLK_RATE             = 100000000,
    parameter MAG_WIDTH               = 26,
    parameter SAMPLES_PER_TURN        = 77,
    parameter NADC                    = 4,
    parameter ADC_WIDTH               = 16,
    parameter DATA_WIDTH              = 32,
    parameter TURNS_PER_PT            = 152000,
    parameter CIC_STAGES              = 3,
    parameter CIC_FA_DECIMATE         = 76,
    parameter CIC_SA_DECIMATE         = 1000,
    parameter GPIO_LO_RF_ROW_CAPACITY = -1,
    parameter GPIO_LO_PT_ROW_CAPACITY = -1,
    parameter LO_WIDTH                = 18,
    parameter GAIN_WIDTH              = MAG_WIDTH + 1,
    parameter PRODUCT_WIDTH           = ADC_WIDTH + LO_WIDTH - 1) (
    input                        clk,
    input       [DATA_WIDTH-1:0] gpioData,
    input                        localOscillatorAddressStrobe,
    input                        localOscillatorCsrStrobe,
    output wire [DATA_WIDTH-1:0] localOscillatorCsr,
    input                        sumShiftCsrStrobe,
    output wire [DATA_WIDTH-1:0] sumShiftCsr,
    input  wire                  autotrimCsrStrobe,
    input  wire                  autotrimThresholdStrobe,
    input             [NADC-1:0] autotrimGainStrobes,
    output wire [DATA_WIDTH-1:0] autotrimCsr, autotrimThreshold,
    output wire [DATA_WIDTH-1:0] gainRBK0, gainRBK1, gainRBK2, gainRBK3,
    input                 [63:0] sysTimestamp,
    input                        adcClk,
    input        [ADC_WIDTH-1:0] adc0, adc1, adc2, adc3,
    input                        adcExceedsThreshold, adcUseThisSample,
    output wire                  adcLoSynced,
    input                        evrClk,
    input                        evrFaMarker, evrSaMarker,
    input                 [63:0] evrTimestamp,
    input                        evrPtTrigger,evrSinglePassTrigger,evrHbMarker,
    output wire                  PT_P, PT_N,
    output reg                   sysSingleTrig,
    output wire  [LO_WIDTH-1:0]  rfLOcosDbg,
    output wire  [LO_WIDTH-1:0]  rfLOsinDbg,
    output wire  [LO_WIDTH-1:0]  plLOcosDbg,
    output wire  [LO_WIDTH-1:0]  plLOsinDbg,
    output wire  [LO_WIDTH-1:0]  phLOcosDbg,
    output wire  [LO_WIDTH-1:0]  phLOsinDbg,
    output wire                  tbtToggle,
    output wire  [MAG_WIDTH-1:0] rfTbtMag0, rfTbtMag1, rfTbtMag2, rfTbtMag3,
    output wire                  faToggle,
    output wire  [MAG_WIDTH-1:0] rfFaMag0, rfFaMag1, rfFaMag2, rfFaMag3,
    output wire                  adcTbtLoadAccumulator,
    output wire                  adcTbtLatchAccumulator,
    output wire                  adcMtLoadAndLatch,
    output reg                   saToggle,
    output reg            [63:0] sysSaTimestamp,
    output reg   [MAG_WIDTH-1:0] rfMag0, rfMag1, rfMag2, rfMag3,
    output reg   [MAG_WIDTH-1:0] plMag0, plMag1, plMag2, plMag3,
    output reg   [MAG_WIDTH-1:0] phMag0, phMag1, phMag2, phMag3,
    output reg                   ptToggle,
    output reg                   overflowFlag);

wire sysUseRMS           = localOscillatorCsr[2];
wire sysIsSinglePassMode = localOscillatorCsr[1];

//
// Local oscillator
//
reg adcHoldoff = 0;
wire [LO_WIDTH-1:0] adcRfCos, adcRfSin, adcPlCos, adcPlSin, adcPhCos, adcPhSin;
(*mark_debug="false"*)
localOscillator #(.OUTPUT_WIDTH(LO_WIDTH),
                  .GPIO_LO_RF_ROW_CAPACITY(GPIO_LO_RF_ROW_CAPACITY),
                  .GPIO_LO_PT_ROW_CAPACITY(GPIO_LO_PT_ROW_CAPACITY))
  localOscillator(.clk(adcClk),
                  .adcSyncMarker(adcSyncMarker),
                  .singleStart(1'b0),
                  .sysClk(clk),
                  .sysAddressStrobe(localOscillatorAddressStrobe),
                  .sysGpioStrobe(localOscillatorCsrStrobe),
                  .sysGpioData(gpioData),
                  .sysGpioCsr(localOscillatorCsr),
                  .tbtLoadAccumulator(adcTbtLoadAccumulator),
                  .tbtLatchAccumulator(adcTbtLatchAccumulator),
                  .mtLoadAndLatch(adcMtLoadAndLatch),
                  .loSynced(adcLoSynced),
                  .rfCos(adcRfCos),
                  .rfSin(adcRfSin),
                  .plCos(adcPlCos),
                  .plSin(adcPlSin),
                  .phCos(adcPhCos),
                  .phSin(adcPhSin));

assign rfLOcosDbg = adcRfCos;
assign rfLOsinDbg = adcRfSin;
assign plLOcosDbg = adcPlCos;
assign plLOsinDbg = adcPlSin;
assign phLOcosDbg = adcPhCos;
assign phLOsinDbg = adcPhSin;

endmodule
