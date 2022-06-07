proc gen_vitis_platform {platform_name xsa_file} {
    # Set workspace
    setws ./$platform_name
    cd $platform_name

    # Create BSP
    app create -name $platform_name -hw $xsa_file -proc psu_cortexa53_0 -os standalone -lang c -template {Empty Application}

    # Add libraries to BSP
    bsp setlib -name xilffs
    bsp setlib -name xilpm
    bsp setlib -name libmetal
    bsp setlib -name lwip211

    # LwIP overrides
    bsp config mem_size 1048576
    bsp config pbuf_pool_size 4096
    bsp config memp_n_udp_pcb 32
    bsp config n_rx_descriptors 128
    bsp config n_tx_descriptors 128
    # bsp config lwip_stats true
    # bsp config lwip_debug true
    # bsp config pbuf_debug true

    # Generate platform
    platform generate
}

if { $argc < 2 } {
    puts "Not enough arguments"
    puts "Usage: xcst gen_vitis_platform.tcl <platform_name> <xsa_file>"
    exit
}

set platform_name [lindex $argv 0]
set xsa_file [lindex $argv 1]

gen_vitis_platform $platform_name $xsa_file
