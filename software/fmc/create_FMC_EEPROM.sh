#!/bin/sh

# All serial numbers the same for now -- get serial number from punch-out block.

set -ex

export PYTHONPATH="$PYTHONPATH=:$HOME/src/FMC/pts_core/tools"
export FRU_VERBOSE=1

python2.7 fru-generator \
                        -o eeprom.bin \
                        -v LBNL \
                        -n "HighSpeedDigitizer" \
                        -p "AFE Rev. A" \
                        -s "000" 
fru-dump -i eeprom.bin -b -c -p -2 -v
