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
#define ADC_CLK_MMCM_MULTIPLIER 104500
#define ADC_CLK_MMCM_DIVIDER      2375

/*
 * Number of ADC AXI clocks per SYSREF clock
 */
#define ADC_CLK_PER_SYSREF  88

/*
 * Number of FPGA_REFCLK_OUT_C clocks per SYSREF clock
 */
#define REFCLK_OUT_PER_SYSREF   2

/*
 * Number of ADC channels required by application
 */
#define CFG_ADC_CHANNEL_COUNT    8

/*
 * ADC sampling clock frequency
 */
#define CFG_ADC_SAMPLING_CLK_FREQ   3997.12

/*
 * ADC reference clock
 */
#define CFG_ADC_REF_CLK_FREQ   3997.12

/*
 * Application-specific registers
 */
#define GPIO_IDX_ADC_0_CSR               32 // Acquisition control(W)/status(R)
#define GPIO_IDX_ADC_0_TRIGGER_CONFIG    33 // Acquisition trigger info (W)
#define GPIO_IDX_ADC_0_TRIGGER_LOCATION  33 // Acquisition trigger location (R)
#define GPIO_IDX_ADC_0_SECONDS           34 // Acquisition trigger time (R)
#define GPIO_IDX_ADC_0_CONFIG_1          34 // Acquisition configuration 1 (W)
#define GPIO_IDX_ADC_0_TICKS             35 // Acquisition trigger time (R)
#define GPIO_IDX_ADC_0_CONFIG_2          35 // Acquisition configuration 2 (W)
#define GPIO_IDX_PER_ADC             4
