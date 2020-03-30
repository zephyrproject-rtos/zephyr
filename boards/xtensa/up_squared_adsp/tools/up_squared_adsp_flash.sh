#!/bin/sh
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

BUILD=$1
FIRMWARE=${BUILD}/zephyr/zephyr.ri
FLASHER=${ZEPHYR_BASE}/boards/xtensa/up_squared_adsp/tools/fw_loader.py

if [ -z "$2" ]
  then
    echo "Signing using default key"
    west sign -d ${BUILD} -t rimage
else
    echo "Signing with key " $key
    west sign -d ${BUILD} -t rimage -- -k $2
fi

${FLASHER} -f ${FIRMWARE} 2>&1
