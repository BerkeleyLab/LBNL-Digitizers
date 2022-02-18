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
