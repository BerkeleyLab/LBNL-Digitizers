# CPLL reference clock constraint (will be overridden by required constraint on IBUFDS_GTE4 input in context). Needed here as we are not using OOC
# build for GTY4 transceiver. The other MGT clocks are already constrained
# by system.bd build
create_clock -period 6.4 [get_ports USER_MGT_SI570_CLK_P]

# RFDC clocks
create_clock -period 87.931 [get_ports FPGA_REFCLK_OUT_C_P]
create_clock -period 175.862 [get_ports SYSREF_FPGA_C_P]

# Don't check timing across clock domains.
set_false_path -from [get_clocks RFADC0_CLK] -to [get_clocks clk_pl_0]
set_false_path -from [get_clocks FPGA_REFCLK_OUT_C_P] -to [get_clocks clk_pl_0]
set_false_path -from [get_clocks SYSREF_FPGA_C_P] -to [get_clocks FPGA_REFCLK_OUT_C_P]
set_false_path -from [get_clocks clk_pl_0] -to [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]]
set_false_path -from [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]] -to [get_clocks clk_pl_0]
set_false_path -from [get_clocks -of_objects [get_pins calibration/prbsMMCM/inst/mmcme4_adv_inst/CLKOUT0]] -to [get_clocks clk_pl_0]
set_false_path -from [get_clocks clk_pl_0] -to [get_clocks -of_objects [get_pins calibration/prbsMMCM/inst/mmcme4_adv_inst/CLKOUT0]]
set_false_path -from [get_clocks FPGA_REFCLK_OUT_C_P] -to [get_clocks -of_objects [get_pins system_i/rfadc_mmcm/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]]
# Set false path between USER_MGT_SI570_CLK_O2 and MGT ref clock,
# only used at the frequencyu meter module
set_false_path -from [get_clocks USER_MGT_SI570_CLK_O2] -to [get_clocks clk_pl_0]
