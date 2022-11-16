/*
 * Configuration values specific to a particular site or  application.
 * Same restrictions as noted in gpio.h.
 */
#define VERILOG_FIRMWARE_STYLE_BCM
#define CFG_ACQUISITION_BUFFER_CAPACITY     (1<<17)
#define CFG_MAX_PASSES_PER_ACQUISITION      (1<<12)

/*
 * ADC AXI MMCM (adcClk source) configuration
 * Values are scaled by a factor of 1000.
 */
#define ADC_CLK_MMCM_MULTIPLIER 105000
#define ADC_CLK_MMCM_DIVIDER      2625
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
#define CFG_ADC_CHANNEL_COUNT    2

/*
 * For softwre compatibility:
 */
#define CFG_ACQ_CHANNEL_COUNT CFG_ADC_CHANNEL_COUNT

/*
 * Number of physical ADC channels required by application
 */
#define CFG_ADC_PHYSICAL_COUNT    2

/*
 * ADC sampling clock frequency
 */
#define CFG_ADC_SAMPLING_CLK_FREQ   3997.12

/*
 * ADC reference clock
 */
#define CFG_ADC_REF_CLK_FREQ   3997.12
