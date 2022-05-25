#!/bin/sh

# Convert TICS '~/lmxxxx.tcs' to C header

set -eu

TCSFILE="$1"

BASENAME=`basename --suffix=.tcs $TCSFILE`
HFILE="$BASENAME.h"

tr -d '\r' <"$TCSFILE" | sed -e 's/^/    \/\/ /'
tr -d '\r' <"$TCSFILE" |
sed -n -e '/^VALUE[0-9]*=\([0-9]*\)/s//    \1,/p'
