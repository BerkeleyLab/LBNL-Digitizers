#!/bin/sh

IP="${HSD_IP_ADDRESS=192.168.1.136}"
SRC="eeprom.bin"

for i
do
    case "$i" in
        *.bin) SRC="$i" ;;
        *)     IP="$i" ;;
    esac
done

set -ex
test -r "$SRC"
tftp -v -m binary "$IP" -c put "$SRC" AFEEPROM.bin
tftp -v -m binary "$IP" -c get AFEEPROM.bin chk.bin
set +e
if cmp "$SRC" chk.bin
then
    echo "Write succeeded."
    fru-dump -i chk.bin -b -c -p -2 -v
    rm chk.bin
else
    echo "Write may have failed!"
fi

