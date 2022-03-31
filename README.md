High Speed Digitizer
====================

Gateware/Software for High Speed digitizer

### Building

This repository contains both gateware and software
for the High Speed Digitizer project.

#### Gateware Dependencies

To build the gateware the following dependencies are needed:

* GNU Make
* Xilinx Vivado (2020.1.1 tested), available [here](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools.html)
* Xilinx Vitis (2020.1.1. tested), available [here](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis.html)

#### Software Dependencies

To build the software the following dependencies are needed:

* aarch64-none toolchain, bundled within Vitis

#### Building Instructions

With the dependencies in place a simple `make` should be able to generate
both gateware and software, plus the SD boot image .bin.

```bash
make
```

Remember that vivado and the ARM toolchain should be in your PATH. If that's
not the case you can pass the ARM directory in the `CROSS_COMPILE` make
variable:

```bash
make CROSS_COMPILE=<ARM_TOOLCHAIN_LOCATION>/bin/aarch64-none-elf-
```

A suggestion in running the `make` command is to measure the time
and redirect stdout/stderr to a file so you can inspect it later:

```bash
ARM_TOOLCHAIN_LOCATION=/media/Xilinx/Vivado/2020.1/Vitis/2020.1/gnu/aarch64/lin/aarch64-none
(time make CROSS_COMPILE=${ARM_TOOLCHAIN_LOCATION}/bin/aarch64-none-elf-; date) 2>&1 | tee make_output
```

### Deploying

To deploy the gateware and the software we can use a variety of
methods. For development, JTAG is being used. Remember to check
the DIP switches on development boards and ensure the switches
are set to JTAG mode and NOT SD Card mode.

#### Deploying gateware

The following script can download the gateware via JTAG:

```bash
cd gateware/scripts
xsct download_bit.tcl ../syn/hsd_zcu111/hsd_zcu111_top.bit
```

#### Deploying software

The following script can download the software via JTAG:

```bash
cd software/scripts
xsct download_elf.tcl ../../gateware/syn/hsd_zcu111/psu_init.tcl ../hsd/HighSpeedDigitizer.elf
```
