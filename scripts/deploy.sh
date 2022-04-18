#!/usr/bin/env bash

set -euo pipefail

APP=${1:-bpm}
PLATFORM=${2:-zcu208}

SCRIPTPATH="$( cd "$( dirname "${BASH_SOURCE[0]}"  )" && pwd  )"

cd ${SCRIPTPATH}/../gateware/scripts
xsct download_bit.tcl \
    ../syn/${APP}_${PLATFORM}/${APP}_${PLATFORM}_top.bit

cd ${SCRIPTPATH}/../software/scripts
xsct download_elf.tcl \
    ../../gateware/syn/${APP}_${PLATFORM}/psu_init.tcl \
    ../app/${APP}/${APP}_${PLATFORM}.elf
