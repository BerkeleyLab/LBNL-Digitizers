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
#define GPIO_IDX_EVR_FA_RELOAD           19 // Fast acquisition divider reload
#define GPIO_IDX_EVR_SA_RELOAD           20 // Slow acquisition divider reload
#define GPIO_IDX_ADC_FA_RELOAD           21 // Fast acquisition divider reload
#define GPIO_IDX_ADC_SA_RELOAD           22 // Slow acquisition divider reload
#define GPIO_IDX_ADC_HEARTBEAT_RELOAD    23 // ADC heartbeat counter

/*
 * Preliminary processing chain
 */
#define GPIO_IDX_LOTABLE_ADDRESS         32 // Local oscillator table write address
#define GPIO_IDX_LOTABLE_CSR             33 // Local oscillator tables
#define GPIO_IDX_SUM_SHIFT_CSR           34 // Accumulator scaling values
#define GPIO_IDX_AUTOTRIM_CSR            35 // Auto gain compensation control/status
#define GPIO_IDX_AUTOTRIM_THRESHOLD      36 // Auto gain compensation tone threshold
#define GPIO_IDX_ADC_GAIN_FACTOR_0       37 // Gain factor for ADC 0
#define GPIO_IDX_ADC_GAIN_FACTOR_1       38 // Gain factor for ADC 1
#define GPIO_IDX_ADC_GAIN_FACTOR_2       39 // Gain factor for ADC 2
#define GPIO_IDX_ADC_GAIN_FACTOR_3       40 // Gain factor for ADC 3
#define GPIO_IDX_PRELIM_RF_MAG_0         41 // ADC 0 RF (SA) magnitude
#define GPIO_IDX_PRELIM_RF_MAG_1         42 // ADC 1 RF (SA) magnitude
#define GPIO_IDX_PRELIM_RF_MAG_2         43 // ADC 2 RF (SA) magnitude
#define GPIO_IDX_PRELIM_RF_MAG_3         44 // ADC 3 RF (SA) magnitude
#define GPIO_IDX_PRELIM_PT_LO_MAG_0      45 // ADC 0 low freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_LO_MAG_1      46 // ADC 1 low freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_LO_MAG_2      47 // ADC 2 low freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_LO_MAG_3      48 // ADC 3 low freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_HI_MAG_0      49 // ADC 0 high freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_HI_MAG_1      50 // ADC 1 high freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_HI_MAG_2      51 // ADC 2 high freq pilot tone magnitude
#define GPIO_IDX_PRELIM_PT_HI_MAG_3      52 // ADC 3 high freq pilot tone magnitude
#define GPIO_IDX_SA_TIMESTAMP_SEC        53 // Slow acquisition time stamp
#define GPIO_IDX_SA_TIMESTAMP_TICKS      54 // Slow acquisition time stamp
#define GPIO_IDX_POSITION_CALC_CSR       55 // Position calculation control/status
#define GPIO_IDX_POSITION_CALC_XCAL      56 // X calibration factor
#define GPIO_IDX_POSITION_CALC_YCAL      57 // Y calibration factor
#define GPIO_IDX_POSITION_CALC_QCAL      58 // Q calibration factor
#define GPIO_IDX_POSITION_CALC_SA_X      59 // Slow acquisition X position
#define GPIO_IDX_POSITION_CALC_SA_Y      60 // Slow acquisition Y position
#define GPIO_IDX_POSITION_CALC_SA_Q      61 // Slow acquisition skew
#define GPIO_IDX_POSITION_CALC_SA_S      62 // Slow acquisition sum

#define GPIO_IDX_PER_BPM                 (GPIO_IDX_POSITION_CALC_SA_S-GPIO_IDX_LOTABLE_ADDRESS+1)

/*
 * Application-specific registers
 */
#define GPIO_IDX_ADC_0_CSR               128 // Acquisition control(W)/status(R)
#define GPIO_IDX_ADC_0_DATA              129 // Acquisition data(R)
#define GPIO_IDX_ADC_0_TRIGGER_CONFIG    130 // Acquisition trigger info (W)
#define GPIO_IDX_ADC_0_TRIGGER_LOCATION  130 // Acquisition trigger location (R)
#define GPIO_IDX_ADC_0_SECONDS           131 // Acquisition trigger time (R)
#define GPIO_IDX_ADC_0_CONFIG_1          131 // Acquisition configuration 1 (W)
#define GPIO_IDX_ADC_0_TICKS             132 // Acquisition trigger time (R)
#define GPIO_IDX_ADC_0_CONFIG_2          132 // Acquisition configuration 2 (W)
#define GPIO_IDX_PER_ADC                 (GPIO_IDX_ADC_0_CONFIG_2-GPIO_IDX_ADC_0_CSR+1)

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
