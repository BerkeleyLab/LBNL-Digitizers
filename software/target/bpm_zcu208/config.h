/*
 * Configuration values specific to a particular site or  application.
 * Same restrictions as noted in gpio.h.
 */
#define VERILOG_FIRMWARE_STYLE_HSD
#define CFG_ACQUISITION_BUFFER_CAPACITY     (1<<14)
#define CFG_LONG_SEGMENT_CAPACITY           (1<<9)
#define CFG_SHORT_SEGMENT_CAPACITY          (1<<6)
#define CFG_EARLY_SEGMENTS_COUNT            5
#define CFG_SEGMENT_PRETRIGGER_COUNT        32
#define CFG_ADCS_PER_BONDED_GROUP           4

/*
 * ADC AXI MMCM (adcClk source) configuration
 * Values are scaled by a factor of 1000.
 */
#define ADC_CLK_MMCM_MULTIPLIER   3750
#define ADC_CLK_MMCM_DIVIDER      12500
#define ADC_CLK_MMCM_CLK1_DIVIDER 3000

/* For compatibility */
#define ADC_CLK_MMCM_CLK0_DIVIDER ADC_CLK_MMCM_DIVIDER

/*
 * Number of ADC AXI clocks per SYSREF clock
 */
#define ADC_CLK_PER_SYSREF  30

/*
 * Number of FPGA_REFCLK_OUT_C clocks per SYSREF clock
 */
#define REFCLK_OUT_PER_SYSREF   100

/*
 * Number of ADC streams required by application
 */
#define CFG_ADC_CHANNEL_COUNT    16 // I/Q

/*
 * Number of physical ADC channels required by application
 */
#define CFG_ADC_PHYSICAL_COUNT    8

/*
 * ADC sampling clock frequency
 */
#define CFG_ADC_SAMPLING_CLK_FREQ   4800.00

/*
 * ADC reference clock
 */
#define CFG_ADC_REF_CLK_FREQ   4800.00

/*
 * Application-specific registers
 */
#define GPIO_IDX_ADC_0_CSR               64 // Acquisition control(W)/status(R)
#define GPIO_IDX_ADC_0_DATA              65 // Acquisition data(R)
#define GPIO_IDX_ADC_0_TRIGGER_CONFIG    66 // Acquisition trigger info (W)
#define GPIO_IDX_ADC_0_TRIGGER_LOCATION  66 // Acquisition trigger location (R)
#define GPIO_IDX_ADC_0_SECONDS           67 // Acquisition trigger time (R)
#define GPIO_IDX_ADC_0_CONFIG_1          67 // Acquisition configuration 1 (W)
#define GPIO_IDX_ADC_0_TICKS             68 // Acquisition trigger time (R)
#define GPIO_IDX_ADC_0_CONFIG_2          68 // Acquisition configuration 2 (W)
#define GPIO_IDX_PER_ADC             5
