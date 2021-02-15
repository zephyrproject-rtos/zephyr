#!/bin/sh
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

BUILD=$1
FIRMWARE=${BUILD}/zephyr/zephyr.ri
FLASHER=${ZEPHYR_BASE}/boards/xtensa/intel_adsp_cavs15/tools/fw_loader.py

if [ -z "$2" ]
  then
    echo "Signing using default key"
    west sign -d ${BUILD} -t rimage
elif [ -n "$3" ] && [ -n "$4" ]
  then
    echo "Signing with key " $key
    west sign -d ${BUILD} -t rimage -p $4 -D $3 -- -k $2 --no-manifest
fi
echo ${FLASHER} -f ${FIRMWARE}
${FLASHER} -f ${FIRMWARE} || /bin/true  2>&1
