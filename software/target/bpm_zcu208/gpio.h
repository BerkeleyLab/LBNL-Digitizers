/*
 * Indices into the big general purpose I/O block.
 * Used to generate Verilog parameter statements too, so be careful with
 * the syntax:
 *     Spaces only (no tabs).
 *     Index defines must be valid Verilog parameter lval expressions.
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#define GPIO_IDX_COUNT 256

#define GPIO_IDX_FIRMWARE_BUILD_DATE      0 // Firmware build POSIX seconds (R)
#define GPIO_IDX_MICROSECONDS_SINCE_BOOT  1 // Microseconds since boot (R)
#define GPIO_IDX_SECONDS_SINCE_BOOT       2 // Seconds since boot (R)
#define GPIO_IDX_USER_GPIO_CSR            3 // Diagnostic LEDS/switches
#define GPIO_IDX_GTY_CSR                  4 // GTY control/status
#define GPIO_IDX_EVR_SYNC_CSR             5 // Event receiver synchronization
#define GPIO_IDX_SYSREF_CSR               6 // SYSREF generation control/status
#define GPIO_IDX_FREQ_MONITOR_CSR         7 // Frequency measurement CSR
#define GPIO_IDX_EVR_GTY_DRP              8 // EVR GTY dynamic reconfig (R/W)
#define GPIO_IDX_SOFT_TRIGGER             9 // Acquisition software trigger (W)
#define GPIO_IDX_DISPLAY_CSR             10 // Display CSR (R/W)
#define GPIO_IDX_DISPLAY_DATA            11 // Display I/O (R/W)
#define GPIO_IDX_AFE_SPI_CSR             12 // AFE SPI devices (R/W)
#define GPIO_IDX_INTERLOCK_CSR           13 // Interlock (R/W)
#define GPIO_IDX_CALIBRATION_CSR         14 // ADC calibration (R/W)
#define GPIO_IDX_EVENT_LOG_CSR           15 // Event logger control/seconds
#define GPIO_IDX_EVENT_LOG_TICKS         16 // Event logger ticks
#define GPIO_IDX_ADC_RANGE_CSR           17 // Monitor ADC ranges
#define GPIO_IDX_CLK104_SPI_MUX_CSR      18 // Select CLK104 SPI MUX
#define GPIO_IDX_LOTABLE_ADDRESS         19 // Local oscillator table write address
#define GPIO_IDX_LOTABLE_CSR             20 // Local oscillator tables
#define GPIO_IDX_SUM_SHIFT_CSR           21 // Accumulator scaling values
#define GPIO_IDX_AUTOTRIM_CSR            22 // Auto gain compensation control/status
#define GPIO_IDX_AUTOTRIM_THRESHOLD      23 // Auto gain compensation tone threshold
#define GPIO_IDX_ADC_GAIN_FACTOR_0       24 // Gain factor for ADC 0
#define GPIO_IDX_ADC_GAIN_FACTOR_1       25 // Gain factor for ADC 1
#define GPIO_IDX_ADC_GAIN_FACTOR_2       26 // Gain factor for ADC 2
#define GPIO_IDX_ADC_GAIN_FACTOR_3       27 // Gain factor for ADC 3
#define GPIO_IDX_PRELIM_RF_MAG_0         28 // ADC 0 RF (SA) magnitude
#define GPIO_IDX_PRELIM_RF_MAG_1         29 // ADC 1 RF (SA) magnitude
#define GPIO_IDX_PRELIM_RF_MAG_2         30 // ADC 2 RF (SA) magnitude
#define GPIO_IDX_PRELIM_RF_MAG_3         31 // ADC 3 RF (SA) magnitude
#define GPIO_IDX_PRELIM_PT_LO_MAG_0      32 // ADC 0 low freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_LO_MAG_1      33 // ADC 1 low freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_LO_MAG_2      34 // ADC 2 low freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_LO_MAG_3      35 // ADC 3 low freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_HI_MAG_0      36 // ADC 0 high freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_HI_MAG_1      37 // ADC 1 high freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_HI_MAG_2      38 // ADC 2 high freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_HI_MAG_3      39 // ADC 3 high freq pilot tone magnitude
#define GPIO_IDX_SA_TIMESTAMP_SEC        40 // Slow acquisition time stamp
#define GPIO_IDX_SA_TIMESTAMP_TICKS      41 // Slow acquisition time stamp
#define GPIO_IDX_EVR_FA_RELOAD           42 // Fast acquisition divider reload
#define GPIO_IDX_EVR_SA_RELOAD           43 // Slow acquisition divider reload
#define GPIO_IDX_ADC_FA_RELOAD           44 // Fast acquisition divider reload
#define GPIO_IDX_ADC_SA_RELOAD           45 // Slow acquisition divider reload
#define GPIO_IDX_ADC_HEARTBEAT_RELOAD    46 // ADC heartbeat counter

#define CFG_AXI_SAMPLES_PER_CLOCK         1 // 1 sample per clock
#define CFG_LO_RF_ROW_CAPACITY           1024
#define CFG_LO_PT_ROW_CAPACITY           8192

#include <xil_io.h>
#include <xparameters.h>
#include "config.h"

#define GPIO_READ(i)    Xil_In32(XPAR_AXI_LITE_GENERIC_REG_0_BASEADDR+(4*(i)))
#define GPIO_WRITE(i,x) Xil_Out32(XPAR_AXI_LITE_GENERIC_REG_0_BASEADDR+(4*(i)),(x))
#define MICROSECONDS_SINCE_BOOT()   GPIO_READ(GPIO_IDX_MICROSECONDS_SINCE_BOOT)

#define USER_GPIO_CSR_RECOVERY_MODE_BUTTON  0x80000000
#define USER_GPIO_CSR_DISPLAY_MODE_BUTTON   0x40000000
#define USER_GPIO_CSR_HEARTBEAT             0x10000

#define EVR_SYNC_CSR_HB_GOOD    0x2
#define EVR_SYNC_CSR_PPS_GOOD   0x4

#endif
