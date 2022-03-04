# SFPs
set_property PACKAGE_PIN AA38 [get_ports SFP0_RX_P]
set_property PACKAGE_PIN AA39 [get_ports SFP0_RX_N]
set_property PACKAGE_PIN Y35 [get_ports SFP0_TX_P]
set_property PACKAGE_PIN Y36 [get_ports SFP0_TX_N]
set_property PACKAGE_PIN G12 [get_ports SFP0_TX_ENABLE]
set_property IOSTANDARD LVCMOS12 [get_ports SFP0_TX_ENABLE]

# USER_MGT_SI570
set_property PACKAGE_PIN V32 [get_ports USER_MGT_SI570_CLK_N]
set_property PACKAGE_PIN V31 [get_ports USER_MGT_SI570_CLK_P]

# CPLL reference clock constraint (will be overridden by required constraint on IBUFDS_GTE4 input in context). Needed here as we are not using OOC
# build for GTY4 transceiver. The other MGT clocks are already constrained
# by system.bd build
create_clock -period 6.4 [get_ports USER_MGT_SI570_CLK_P]

# Set false path between USER_MGT_SI570_CLK_O2 and MGT ref clock,
# only used at the frequencyu meter module
set_false_path -from [get_clocks USER_MGT_SI570_CLK_O2] -to [get_clocks clk_pl_0]

# User DIP switch
set_property PACKAGE_PIN AF16 [get_ports {DIP_SWITCH[0]}]
set_property PACKAGE_PIN AF17 [get_ports {DIP_SWITCH[1]}]
set_property PACKAGE_PIN AH15 [get_ports {DIP_SWITCH[2]}]
set_property PACKAGE_PIN AH16 [get_ports {DIP_SWITCH[3]}]
set_property PACKAGE_PIN AH17 [get_ports {DIP_SWITCH[4]}]
set_property PACKAGE_PIN AG17 [get_ports {DIP_SWITCH[5]}]
set_property PACKAGE_PIN AJ15 [get_ports {DIP_SWITCH[6]}]
set_property PACKAGE_PIN AJ16 [get_ports {DIP_SWITCH[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DIP_SWITCH[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DIP_SWITCH[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DIP_SWITCH[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DIP_SWITCH[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DIP_SWITCH[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DIP_SWITCH[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DIP_SWITCH[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DIP_SWITCH[7]}]

# Analog signal conditioning card
set_property PACKAGE_PIN AR7 [get_ports AFE_SPI_CLK]
set_property PACKAGE_PIN AP6 [get_ports AFE_SPI_SDI]
set_property PACKAGE_PIN AR6 [get_ports AFE_SPI_SDO]
set_property PACKAGE_PIN AU7 [get_ports {AFE_SPI_CSB[0]}]
set_property PACKAGE_PIN AV8 [get_ports {AFE_SPI_CSB[1]}]
set_property PACKAGE_PIN AU8 [get_ports {AFE_SPI_CSB[2]}]
set_property PACKAGE_PIN AT6 [get_ports {AFE_SPI_CSB[3]}]
set_property PACKAGE_PIN AT7 [get_ports {AFE_SPI_CSB[4]}]
set_property PACKAGE_PIN AU5 [get_ports {AFE_SPI_CSB[5]}]
set_property PACKAGE_PIN AT5 [get_ports {AFE_SPI_CSB[6]}]
set_property PACKAGE_PIN AU3 [get_ports {AFE_SPI_CSB[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports AFE_SPI_CLK]
set_property IOSTANDARD LVCMOS18 [get_ports AFE_SPI_SDI]
set_property IOSTANDARD LVCMOS18 [get_ports AFE_SPI_SDO]
set_property IOSTANDARD LVCMOS18 [get_ports {AFE_SPI_CSB[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {AFE_SPI_CSB[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {AFE_SPI_CSB[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {AFE_SPI_CSB[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {AFE_SPI_CSB[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {AFE_SPI_CSB[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {AFE_SPI_CSB[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {AFE_SPI_CSB[7]}]
set_property PACKAGE_PIN AU4 [get_ports TRAINING_SIGNAL]
set_property IOSTANDARD LVCMOS18 [get_ports TRAINING_SIGNAL]
set_property DRIVE 16 [get_ports TRAINING_SIGNAL]
set_property -dict {PACKAGE_PIN A9 IOSTANDARD LVCMOS18 DRIVE 16} [get_ports AFE_DACIO_00]

# DACIO_19 -- GND for XM500 front end SROC jumper
set_property PACKAGE_PIN E8 [get_ports BCM_SROC_GND]
set_property IOSTANDARD LVCMOS18 [get_ports BCM_SROC_GND]


# User push buttons
set_property PACKAGE_PIN AW6 [get_ports GPIO_SW_W]
set_property PACKAGE_PIN AW4 [get_ports GPIO_SW_E]
set_property PACKAGE_PIN AW3 [get_ports GPIO_SW_N]
set_property IOSTANDARD LVCMOS18 [get_ports GPIO_SW_W]
set_property IOSTANDARD LVCMOS18 [get_ports GPIO_SW_E]
set_property IOSTANDARD LVCMOS18 [get_ports GPIO_SW_N]

# GPIO LEDs
set_property PACKAGE_PIN AR13 [get_ports {GPIO_LEDS[0]}]
set_property PACKAGE_PIN AP13 [get_ports {GPIO_LEDS[1]}]
set_property PACKAGE_PIN AR16 [get_ports {GPIO_LEDS[2]}]
set_property PACKAGE_PIN AP16 [get_ports {GPIO_LEDS[3]}]
set_property PACKAGE_PIN AP15 [get_ports {GPIO_LEDS[4]}]
set_property PACKAGE_PIN AN16 [get_ports {GPIO_LEDS[5]}]
set_property PACKAGE_PIN AN17 [get_ports {GPIO_LEDS[6]}]
set_property PACKAGE_PIN AV15 [get_ports {GPIO_LEDS[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {GPIO_LEDS[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {GPIO_LEDS[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {GPIO_LEDS[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {GPIO_LEDS[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {GPIO_LEDS[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {GPIO_LEDS[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {GPIO_LEDS[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {GPIO_LEDS[7]}]

# PMOD 1 connector -- Display
set_property PACKAGE_PIN L14 [get_ports PMOD1_0]
set_property PACKAGE_PIN L15 [get_ports PMOD1_1]
set_property PACKAGE_PIN M13 [get_ports PMOD1_2]
set_property PACKAGE_PIN N13 [get_ports PMOD1_3]
set_property PACKAGE_PIN M15 [get_ports PMOD1_4]
set_property PACKAGE_PIN N15 [get_ports PMOD1_5]
set_property PACKAGE_PIN M14 [get_ports PMOD1_6]
set_property PACKAGE_PIN N14 [get_ports PMOD1_7]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD1_0]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD1_1]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD1_2]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD1_3]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD1_4]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD1_5]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD1_6]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD1_7]
set_property PULLUP true [get_ports PMOD1_6]
set_property PULLUP true [get_ports PMOD1_7]
set_property DRIVE 8 [get_ports PMOD1_1]

# PMOD 0 connector -- Interlocks
set_property PACKAGE_PIN C17 [get_ports PMOD0_0]
set_property PACKAGE_PIN M18 [get_ports PMOD0_1]
set_property PACKAGE_PIN H16 [get_ports PMOD0_2]
set_property PACKAGE_PIN H17 [get_ports PMOD0_3]
set_property PACKAGE_PIN J16 [get_ports PMOD0_4]
set_property PACKAGE_PIN K16 [get_ports PMOD0_5]
set_property PACKAGE_PIN H15 [get_ports PMOD0_6]
set_property PACKAGE_PIN J15 [get_ports PMOD0_7]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD0_0]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD0_1]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD0_2]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD0_3]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD0_4]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD0_5]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD0_6]
set_property IOSTANDARD LVCMOS12 [get_ports PMOD0_7]

# EVR recovered clock output
set_property PACKAGE_PIN AP14 [get_ports AMS_FPGA_REF_CLK]
set_property IOSTANDARD LVCMOS18 [get_ports AMS_FPGA_REF_CLK]

# Jitter cleaner monitoring
# Assume ALS-U AR/SR RF (125.0980659 MHz)
set_property PACKAGE_PIN AK17 [get_ports SYSREF_FPGA_C_P]
set_property PACKAGE_PIN AK16 [get_ports SYSREF_FPGA_C_N]
set_property IOSTANDARD LVDS [get_ports SYSREF_FPGA_C_P]
set_property IOSTANDARD LVDS [get_ports SYSREF_FPGA_C_N]
set_property DIFF_TERM_ADV TERM_100 [get_ports SYSREF_FPGA_C_P]
set_property DIFF_TERM_ADV TERM_100 [get_ports SYSREF_FPGA_C_N]
set_property PACKAGE_PIN AL16 [get_ports FPGA_REFCLK_OUT_C_P]
set_property PACKAGE_PIN AL15 [get_ports FPGA_REFCLK_OUT_C_N]
set_property IOSTANDARD LVDS [get_ports FPGA_REFCLK_OUT_C_P]
set_property IOSTANDARD LVDS [get_ports FPGA_REFCLK_OUT_C_N]
set_property DIFF_TERM_ADV TERM_100 [get_ports FPGA_REFCLK_OUT_C_P]
set_property DIFF_TERM_ADV TERM_100 [get_ports FPGA_REFCLK_OUT_C_N]
create_clock -period 87.931 [get_ports FPGA_REFCLK_OUT_C_P]
create_clock -period 175.862 [get_ports SYSREF_FPGA_C_P]

# RFDC SYSREF
set_property PACKAGE_PIN U4 [get_ports SYSREF_RFSOC_C_N]
set_property PACKAGE_PIN U5 [get_ports SYSREF_RFSOC_C_P]
# set_property IOSTANDARD LVDS [get_ports SYSREF_RFSOC_C_P]
# set_property DIFF_TERM_ADV TERM_100 [get_ports SYSREF_RFSOC_C_P]

# RF ADC tile 224
set_property PACKAGE_PIN AF4 [get_ports RF1_CLKO_A_C_N]
set_property PACKAGE_PIN AF5 [get_ports RF1_CLKO_A_C_P]
set_property PACKAGE_PIN AP1 [get_ports RFMC_ADC_00_N]
set_property PACKAGE_PIN AP2 [get_ports RFMC_ADC_00_P]
set_property PACKAGE_PIN AM1 [get_ports RFMC_ADC_01_N]
set_property PACKAGE_PIN AM2 [get_ports RFMC_ADC_01_P]

# RF ADC tile 225
set_property PACKAGE_PIN AD4 [get_ports RF1_CLKO_B_C_N]
set_property PACKAGE_PIN AD5 [get_ports RF1_CLKO_B_C_P]
set_property PACKAGE_PIN AK1 [get_ports RFMC_ADC_02_N]
set_property PACKAGE_PIN AK2 [get_ports RFMC_ADC_02_P]
set_property PACKAGE_PIN AH1 [get_ports RFMC_ADC_03_N]
set_property PACKAGE_PIN AH2 [get_ports RFMC_ADC_03_P]

# RF ADC tile 226
set_property PACKAGE_PIN AB4 [get_ports RF2_CLKO_B_C_N]
set_property PACKAGE_PIN AB5 [get_ports RF2_CLKO_B_C_P]
set_property PACKAGE_PIN AF1 [get_ports RFMC_ADC_04_N]
set_property PACKAGE_PIN AF2 [get_ports RFMC_ADC_04_P]
set_property PACKAGE_PIN AD1 [get_ports RFMC_ADC_05_N]
set_property PACKAGE_PIN AD2 [get_ports RFMC_ADC_05_P]

# RF ADC tile 227
set_property PACKAGE_PIN Y4 [get_ports RF2_CLKO_A_C_N]
set_property PACKAGE_PIN Y5 [get_ports RF2_CLKO_A_C_P]
set_property PACKAGE_PIN AB1 [get_ports RFMC_ADC_06_N]
set_property PACKAGE_PIN AB2 [get_ports RFMC_ADC_06_P]
set_property PACKAGE_PIN Y1 [get_ports RFMC_ADC_07_N]
set_property PACKAGE_PIN Y2 [get_ports RFMC_ADC_07_P]

# Don't check timing across clock domains.
set_false_path -from [get_clocks RFADC0_CLK] -to [get_clocks clk_pl_0]
set_false_path -from [get_clocks FPGA_REFCLK_OUT_C_P] -to [get_clocks clk_pl_0]
set_false_path -from [get_clocks SYSREF_FPGA_C_P] -to [get_clocks FPGA_REFCLK_OUT_C_P]
set_false_path -from [get_clocks clk_pl_0] -to [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]]
set_false_path -from [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]] -to [get_clocks clk_pl_0]
set_false_path -from [get_clocks -of_objects [get_pins calibration/prbsMMCM/inst/mmcme4_adv_inst/CLKOUT0]] -to [get_clocks clk_pl_0]
set_false_path -from [get_clocks clk_pl_0] -to [get_clocks -of_objects [get_pins calibration/prbsMMCM/inst/mmcme4_adv_inst/CLKOUT0]]
set_false_path -from [get_clocks FPGA_REFCLK_OUT_C_P] -to [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]]
