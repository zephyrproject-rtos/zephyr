#!/bin/sh

# This script is loosly based on a script with same purpose provided
# by RIOT-OS (https://github.com/RIOT-OS/RIOT)

BOSSAC_CMD="${BOSSAC:-bossac}"
if [ `uname` = "Linux" ]; then
    stty -F /dev/ttyACM0 raw ispeed 1200 ospeed 1200 cs8 \
-cstopb ignpar eol 255 eof 255
    ${BOSSAC} -R -e -w -v -b "${O}/${KERNEL_BIN_NAME}"
else
    echo "CAUTION: No flash tool for your host system found!"
fi

