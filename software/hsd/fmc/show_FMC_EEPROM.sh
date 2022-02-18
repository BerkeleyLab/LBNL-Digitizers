#!/bin/sh

IP="${HSD_IP_ADDRESS=192.168.1.136}"

for i
do
    case "$i" in
        *)     IP="$i" ;;
    esac
done

set -ex
tftp -v -m binary "$IP" -c get AFEEPROM.bin chk.bin
fru-dump -i chk.bin -b -c -p -2 -v
rm chk.bin
