# CPLL reference clock constraint (will be overridden by required constraint on IBUFDS_GTE4 input in context). Needed here as we are not using OOC
# build for GTY4 transceiver. The other MGT clocks are already constrained
# by system.bd build
create_clock -period 6.4 [get_ports USER_MGT_SI570_CLK_P]

# RFDC clocks
# 412 MHz
create_clock -period 2.42 [get_ports FPGA_REFCLK_OUT_C_P]
# 4.12 MHz
create_clock -period 242.00 [get_ports SYSREF_FPGA_C_P]

set clk_pl_0_period                   [get_property PERIOD [get_clocks clk_pl_0]]
set clk_cpllpd_int_reg_0_period       [get_property PERIOD [get_clocks cpllpd_int_reg_0]]
set clk_prbs_period                   [get_property PERIOD [get_clocks -of_objects [get_pins calibration/prbsMMCM/inst/mmcme4_adv_inst/CLKOUT0]]]
set clk_drp_period                    [get_property PERIOD [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]]]

#########################################
# Set max delay constraints for clock
# to/from sys with 1 destination clock
# period
#########################################
set_max_delay -datapath_only -from [get_clocks RFADC0_CLK] -to [get_clocks clk_pl_0] $clk_pl_0_period
set_max_delay -datapath_only -from [get_clocks FPGA_REFCLK_OUT_C_P] -to [get_clocks clk_pl_0] $clk_pl_0_period

set_max_delay -datapath_only -from [get_clocks clk_pl_0] -to [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]] $clk_drp_period
set_max_delay -datapath_only -from [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]] -to [get_clocks clk_pl_0] $clk_pl_0_period
set_max_delay -datapath_only -from [get_clocks -of_objects [get_pins calibration/prbsMMCM/inst/mmcme4_adv_inst/CLKOUT0]] -to [get_clocks clk_pl_0] $clk_pl_0_period
set_max_delay -datapath_only -from [get_clocks clk_pl_0] -to [get_clocks -of_objects [get_pins calibration/prbsMMCM/inst/mmcme4_adv_inst/CLKOUT0]] $clk_prbs_period

# Set max delay path between USER_MGT_SI570_CLK_O2 and MGT ref clock,
# only used at the frequency meter module
set_max_delay -datapath_only -from [get_clocks USER_MGT_SI570_CLK_O2] -to [get_clocks clk_pl_0] $clk_pl_0_period

# Set max delay path between SYS clock and RX MGT clock. Crossing domains already
# and with ASYNC_REG properties in RTL.
set_max_delay -datapath_only -from [get_clocks clk_pl_0] -to [get_clocks cpllpd_int_reg_0] $clk_cpllpd_int_reg_0_period
set_max_delay -datapath_only -from [get_clocks cpllpd_int_reg_0] -to [get_clocks clk_pl_0] $clk_pl_0_period

#########################################
# Don't check timing across clock domains.
#########################################
set_false_path -from [get_clocks SYSREF_FPGA_C_P] -to [get_clocks FPGA_REFCLK_OUT_C_P]
set_false_path -from [get_clocks FPGA_REFCLK_OUT_C_P] -to [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]]
# Set false path between ADC clock and RX MGT clock. Safely crossing domains already
# and with ASYNC_REG properties in RTL.
set_false_path -from [get_clocks clk_out1_system_rfadc_mmcm_0] -to [get_clocks cpllpd_int_reg_0]
set_false_path -from [get_clocks cpllpd_int_reg_0] -to [get_clocks clk_out1_system_rfadc_mmcm_0]
