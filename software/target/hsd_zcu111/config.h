/*
 * Configuration values specific to a particular site or  application.
 * Same restrictions as noted in gpio.h.
 */
#define VERILOG_FIRMWARE_STYLE_HSD
#define CFG_ACQUISITION_BUFFER_CAPACITY     (1<<17)
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
/* Unused, placeholder */
#define ADC_CLK_MMCM_CLK1_DIVIDER 1000

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
 * For softwre compatibility:
 */
#define CFG_ACQ_CHANNEL_COUNT CFG_ADC_CHANNEL_COUNT

/*
 * Number of physical ADC channels required by application
 */
#define CFG_ADC_PHYSICAL_COUNT    8

/*
 * ADC sampling clock frequency
 */
#define CFG_ADC_SAMPLING_CLK_FREQ   3997.12

/*
 * ADC reference clock
 */
#define CFG_ADC_REF_CLK_FREQ   3997.12
