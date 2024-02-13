#!/bin/sh

# All serial numbers the same for now -- get serial number from punch-out block.

# For now, you would need to clone the pts_core repository
# available here: https://gitlab.lbl.gov/alsu-instrumentation/pts_core
# and set it here

set -ex

export PTS_CORE_DIR=""
export PYTHONPATH="$PYTHONPATH=:$PTS_CORE_DIR/tools"
export FRU_VERBOSE=1

python2.7 fru-generator \
                        -o eeprom.bin \
                        -v LBNL \
                        -n "HighSpeedDigitizer" \
                        -p "AFE Rev. A" \
                        -s "000"
fru-dump -i eeprom.bin -b -c -p -2 -v
