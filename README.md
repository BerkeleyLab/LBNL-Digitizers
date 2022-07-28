Digitizers
==========

Gateware/Software for High Speed digitizers, including
High Speed Digitizer and Beam Position Monitor

### Building

This repository contains both gateware and software
for some High Speed digitizers.

#### Gateware Dependencies

To build the gateware the following dependencies are needed:

* GNU Make
* Xilinx Vivado (2020.1.1 tested), available [here](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools.html)
* Xilinx Vitis (2020.1.1. tested), available [here](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis.html)

Make sure `vivado` and `vitis` are in PATH.

#### Software Dependencies

To build the software the following dependencies are needed:

* aarch64-none toolchain, bundled within Vitis

#### Building Instructions

With the dependencies in place a simple `make` should be able to generate
both gateware and software, plus the SD boot image .bin.

```bash
make
```

A suggestion in running the `make` command is to measure the time
and redirect stdout/stderr to a file so you can inspect it later:

```bash
ARM_TOOLCHAIN_LOCATION=/media/Xilinx/Vivado/2020.1/Vitis/2020.1/gnu/aarch64/lin/aarch64-none
(time make PLATFORM=<PLATFORM_NAME> APP=<APP_NAME> CROSS_COMPILE=${ARM_TOOLCHAIN_LOCATION}/bin/aarch64-none-elf-; date) 2>&1 | tee make_output
```

For now the following combinations of PLATFORM and APP are supported:

| APP / PLATFORM | zcu111 | zcu208 |
|:--------------:|:------:|:------:|
|       hsd      |    x   |    x   |
|       bcm      |    x   |        |
|       bpm      |        |    x   |

So, for example, to generate the HSD application for the ZCU111 board:

```bash
ARM_TOOLCHAIN_LOCATION=/media/Xilinx/Vivado/2020.1/Vitis/2020.1/gnu/aarch64/lin/aarch64-none
(time make PLATFORM=zcu111 APP=hsd CROSS_COMPILE=${ARM_TOOLCHAIN_LOCATION}/bin/aarch64-none-elf-; date) 2>&1 | tee make_output
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
xsct download_elf.tcl ../../gateware/syn/hsd_zcu111/psu_init.tcl ../app/hsd/hsd_zcu111.elf
```
