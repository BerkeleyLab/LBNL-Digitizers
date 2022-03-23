# SFPs - MGT 129 - zSFP 2
set_property PACKAGE_PIN N38        [get_ports SFP2_RX_P]
set_property PACKAGE_PIN N39        [get_ports SFP2_RX_N]
set_property PACKAGE_PIN P35        [get_ports SFP2_TX_P]
set_property PACKAGE_PIN P36        [get_ports SFP2_TX_N]
set_property PACKAGE_PIN AP18       [get_ports SFP2_TX_ENABLE]
set_property IOSTANDARD LVCMOS12    [get_ports SFP2_TX_ENABLE]

# USER_MGT_SI570
set_property PACKAGE_PIN M32        [get_ports USER_MGT_SI570_CLK_N]
set_property PACKAGE_PIN M31        [get_ports USER_MGT_SI570_CLK_P]

# User DIP switch
set_property PACKAGE_PIN M18        [get_ports {DIP_SWITCH[0]}]
set_property PACKAGE_PIN L14        [get_ports {DIP_SWITCH[1]}]
set_property PACKAGE_PIN AF15       [get_ports {DIP_SWITCH[2]}]
set_property PACKAGE_PIN AP15       [get_ports {DIP_SWITCH[3]}]
set_property PACKAGE_PIN N21        [get_ports {DIP_SWITCH[4]}]
set_property PACKAGE_PIN N20        [get_ports {DIP_SWITCH[5]}]
set_property PACKAGE_PIN AW5        [get_ports {DIP_SWITCH[6]}]
set_property PACKAGE_PIN AW6        [get_ports {DIP_SWITCH[7]}]
set_property IOSTANDARD LVCMOS12    [get_ports {DIP_SWITCH[0]}]
set_property IOSTANDARD LVCMOS12    [get_ports {DIP_SWITCH[1]}]
set_property IOSTANDARD LVCMOS18    [get_ports {DIP_SWITCH[2]}]
set_property IOSTANDARD LVCMOS18    [get_ports {DIP_SWITCH[3]}]
set_property IOSTANDARD LVCMOS18    [get_ports {DIP_SWITCH[4]}]
set_property IOSTANDARD LVCMOS18    [get_ports {DIP_SWITCH[5]}]
set_property IOSTANDARD LVCMOS18    [get_ports {DIP_SWITCH[6]}]
set_property IOSTANDARD LVCMOS18    [get_ports {DIP_SWITCH[7]}]

# Analog signal conditioning card
# ADCIO_03
set_property PACKAGE_PIN AR7        [get_ports AFE_SPI_CLK]
# ADCIO_01
set_property PACKAGE_PIN AP6        [get_ports AFE_SPI_SDI]
# ADCIO_02
set_property PACKAGE_PIN AR6        [get_ports AFE_SPI_SDO]
# ADCIO_05
set_property PACKAGE_PIN AU7        [get_ports {AFE_SPI_CSB[0]}]
# ADCIO_06
set_property PACKAGE_PIN AV8        [get_ports {AFE_SPI_CSB[1]}]
#ADCIO_07
set_property PACKAGE_PIN AU8        [get_ports {AFE_SPI_CSB[2]}]
# ADCIO_08
set_property PACKAGE_PIN AT6        [get_ports {AFE_SPI_CSB[3]}]
# ADCIO_09
set_property PACKAGE_PIN AT7        [get_ports {AFE_SPI_CSB[4]}]
# ADCIO_10
set_property PACKAGE_PIN AU5        [get_ports {AFE_SPI_CSB[5]}]
# ADCIO_11
set_property PACKAGE_PIN AT5        [get_ports {AFE_SPI_CSB[6]}]
# ADCIO_12
set_property PACKAGE_PIN AW3        [get_ports {AFE_SPI_CSB[7]}]
set_property IOSTANDARD LVCMOS18    [get_ports AFE_SPI_CLK]
set_property IOSTANDARD LVCMOS18    [get_ports AFE_SPI_SDI]
set_property IOSTANDARD LVCMOS18    [get_ports AFE_SPI_SDO]
set_property IOSTANDARD LVCMOS18    [get_ports {AFE_SPI_CSB[0]}]
set_property IOSTANDARD LVCMOS18    [get_ports {AFE_SPI_CSB[1]}]
set_property IOSTANDARD LVCMOS18    [get_ports {AFE_SPI_CSB[2]}]
set_property IOSTANDARD LVCMOS18    [get_ports {AFE_SPI_CSB[3]}]
set_property IOSTANDARD LVCMOS18    [get_ports {AFE_SPI_CSB[4]}]
set_property IOSTANDARD LVCMOS18    [get_ports {AFE_SPI_CSB[5]}]
set_property IOSTANDARD LVCMOS18    [get_ports {AFE_SPI_CSB[6]}]
set_property IOSTANDARD LVCMOS18    [get_ports {AFE_SPI_CSB[7]}]

# ADCIO_13
set_property PACKAGE_PIN AW4        [get_ports TRAINING_SIGNAL]
set_property IOSTANDARD LVCMOS18    [get_ports TRAINING_SIGNAL]
set_property DRIVE 16               [get_ports TRAINING_SIGNAL]

set_property PACKAGE_PIN A9         [get_ports AFE_DACIO_00]
set_property IOSTANDARD LVCMOS18    [get_ports AFE_DACIO_00]
set_property DRIVE 16               [get_ports AFE_DACIO_00]

# GPIO_SW_S -- GND for XM500 front end SROC jumper
set_property PACKAGE_PIN A25        [get_ports BCM_SROC_GND]
set_property IOSTANDARD LVCMOS18    [get_ports BCM_SROC_GND]

# User push buttons
set_property PACKAGE_PIN L22 [get_ports GPIO_SW_W]
set_property PACKAGE_PIN G24 [get_ports GPIO_SW_E]
set_property PACKAGE_PIN D20 [get_ports GPIO_SW_N]
set_property IOSTANDARD LVCMOS18 [get_ports GPIO_SW_W]
set_property IOSTANDARD LVCMOS18 [get_ports GPIO_SW_E]
set_property IOSTANDARD LVCMOS18 [get_ports GPIO_SW_N]

# GPIO LEDs
set_property PACKAGE_PIN AR19 [get_ports {GPIO_LEDS[0]}]
set_property PACKAGE_PIN AT17 [get_ports {GPIO_LEDS[1]}]
set_property PACKAGE_PIN AR17 [get_ports {GPIO_LEDS[2]}]
set_property PACKAGE_PIN AU19 [get_ports {GPIO_LEDS[3]}]
set_property PACKAGE_PIN AU20 [get_ports {GPIO_LEDS[4]}]
set_property PACKAGE_PIN AW21 [get_ports {GPIO_LEDS[5]}]
set_property PACKAGE_PIN AV21 [get_ports {GPIO_LEDS[6]}]
set_property PACKAGE_PIN AV17 [get_ports {GPIO_LEDS[7]}]
set_property IOSTANDARD LVCMOS12 [get_ports {GPIO_LEDS[0]}]
set_property IOSTANDARD LVCMOS12 [get_ports {GPIO_LEDS[1]}]
set_property IOSTANDARD LVCMOS12 [get_ports {GPIO_LEDS[2]}]
set_property IOSTANDARD LVCMOS12 [get_ports {GPIO_LEDS[3]}]
set_property IOSTANDARD LVCMOS12 [get_ports {GPIO_LEDS[4]}]
set_property IOSTANDARD LVCMOS12 [get_ports {GPIO_LEDS[5]}]
set_property IOSTANDARD LVCMOS12 [get_ports {GPIO_LEDS[6]}]
set_property IOSTANDARD LVCMOS12 [get_ports {GPIO_LEDS[7]}]

# SPI Mux Selection
set_property PACKAGE_PIN C11       [get_ports "CLK_SPI_MUX_SEL0"]
set_property PACKAGE_PIN B12       [get_ports "CLK_SPI_MUX_SEL1"]
set_property IOSTANDARD  LVCMOS12  [get_ports "CLK_SPI_MUX_SEL0"]
set_property IOSTANDARD  POD12_DCI [get_ports "CLK_SPI_MUX_SEL1"]

# EVR recovered clock output. To CLK104 board
# Bank 67
set_property PACKAGE_PIN L21  [get_ports SFP_REC_CLK_N]
set_property PACKAGE_PIN M20  [get_ports SFP_REC_CLK_P]
set_property IOSTANDARD  LVDS [get_ports SFP_REC_CLK_N]
set_property IOSTANDARD  LVDS [get_ports SFP_REC_CLK_P]

# Note on LVDS_25 used on a 1.8V bank:
#
# There is not a specific requirement on the
# VCCO bank voltage on LVDS_25 inputs provided
# the VCCO level is high enough to ensure the
# pin voltage aligns to the Vin spec in the
# Recommended Operating Conditions table of the
# specific UltraScale+ device data sheet

# Jitter cleaner monitoring
# Assume ALS-U AR/SR RF (125.0980659 MHz)
# PL_SYSREF_P - Bank  87 VCCO - VCC1V8   - IO_L8P_HDGC_87
set_property PACKAGE_PIN B10     [get_ports SYSREF_FPGA_C_P]
# PL_SYSREF_N - Bank  87 VCCO - VCC1V8   - IO_L8N_HDGC_87
set_property PACKAGE_PIN B9      [get_ports SYSREF_FPGA_C_N]
set_property IOSTANDARD LVDS_25  [get_ports SYSREF_FPGA_C_P]
set_property IOSTANDARD LVDS_25  [get_ports SYSREF_FPGA_C_N]

# PL_CLK_P - Bank  87 VCCO - VCC1V8   - IO_L7P_HDGC_87
set_property PACKAGE_PIN B8     [get_ports FPGA_REFCLK_OUT_C_P]
# PL_CLK_N - Bank  87 VCCO - VCC1V8   - IO_L7N_HDGC_87
set_property PACKAGE_PIN B7     [get_ports FPGA_REFCLK_OUT_C_N]
set_property IOSTANDARD LVDS_25 [get_ports FPGA_REFCLK_OUT_C_P]
set_property IOSTANDARD LVDS_25 [get_ports FPGA_REFCLK_OUT_C_N]

# As we are using a HDGC pin to drive BUFG -> MMCM, we need
# to use a sub-optimal path in the ZU48. ug572-ultrascale-clocking
# page 10: "Therefore, clocks that are connected to an HDGC pin
# can only connect to MMCMs/PLLs through the BUFGCEs. To avoid
# a design rule check (DRC) error, set the property
# CLOCK_DEDICATED_ROUTE = FALSE."
set_property CLOCK_DEDICATED_ROUTE ANY_CMT_COLUMN [get_nets                    FPGA_REFCLK_OUT_C]

# RFDC SYSREF
# AMS_SYSREF_P - CLK104_AMS_SYSREF_C_P     Bank 228 - SYSREF_P_228
set_property PACKAGE_PIN U5 [get_ports SYSREF_RFSOC_C_P]
# AMS_SYSREF_N - CLK104_AMS_SYSREF_C_N     Bank 228 - SYSREF_N_228
set_property PACKAGE_PIN U4 [get_ports SYSREF_RFSOC_C_N]
# set_property IOSTANDARD LVDS [get_ports SYSREF_RFSOC_C_P]
# set_property DIFF_TERM_ADV TERM_100 [get_ports SYSREF_RFSOC_C_P]

# All ADCs are clocked from the 225

# RF ADC tile 224
set_property PACKAGE_PIN AP1 [get_ports RFMC_ADC_00_N]
set_property PACKAGE_PIN AP2 [get_ports RFMC_ADC_00_P]
set_property PACKAGE_PIN AM1 [get_ports RFMC_ADC_01_N]
set_property PACKAGE_PIN AM2 [get_ports RFMC_ADC_01_P]

# RF ADC tile 225
# ADC_CLK_225_N - Bank 225 - ADC_CLK_N_225
set_property PACKAGE_PIN AD4 [get_ports RF1_CLKO_B_C_N]
# ADC_CLK_225_P - Bank 225 - ADC_CLK_P_225
set_property PACKAGE_PIN AD5 [get_ports RF1_CLKO_B_C_P]
set_property PACKAGE_PIN AK1 [get_ports RFMC_ADC_02_N]
set_property PACKAGE_PIN AK2 [get_ports RFMC_ADC_02_P]
set_property PACKAGE_PIN AH1 [get_ports RFMC_ADC_03_N]
set_property PACKAGE_PIN AH2 [get_ports RFMC_ADC_03_P]

# RF ADC tile 226
set_property PACKAGE_PIN AF1 [get_ports RFMC_ADC_04_N]
set_property PACKAGE_PIN AF2 [get_ports RFMC_ADC_04_P]
set_property PACKAGE_PIN AD1 [get_ports RFMC_ADC_05_N]
set_property PACKAGE_PIN AD2 [get_ports RFMC_ADC_05_P]

# RF ADC tile 227
set_property PACKAGE_PIN AB1 [get_ports RFMC_ADC_06_N]
set_property PACKAGE_PIN AB2 [get_ports RFMC_ADC_06_P]
set_property PACKAGE_PIN Y1 [get_ports RFMC_ADC_07_N]
set_property PACKAGE_PIN Y2 [get_ports RFMC_ADC_07_P]
