if { $argc < 1 } {
    puts "Not enough arguments"
    puts "Usage: xsct download_bit.tcl <bistream>"
    exit
}

set bit_file [file normalize [lindex $argv 0]]

# Heavily based on
# https://www.xilinx.com/htmldocs/xilinx2019_1/SDK_Doc/xsct/use_cases/xsdb_debug_app_zynqmp.html

# Connect
connect
# Select PL unit
targets 3

# Configure the FPGA. When the active target is not a FPGA device,
# the first FPGA device is configured
fpga $bit_file

# cleanup
exit
