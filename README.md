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

* aarch64-none toolchain, available [here](https://developer.arm.com/-/media/Files/downloads/gnu-a/10.3-2021.07/binrel/gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf.tar.xz)

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
ARM_TOOLCHAIN_LOCATION=~/Downloads/gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf
(time make CROSS_COMPILE=${ARM_TOOLCHAIN_LOCATION}/bin/aarch64-none-elf-; date) 2>&1 | tee make_output
```
