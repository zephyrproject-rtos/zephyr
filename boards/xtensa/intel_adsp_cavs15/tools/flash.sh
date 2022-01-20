#!/bin/sh
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

BUILD=$1
FIRMWARE=${BUILD}/zephyr/zephyr.ri
FLASHER=${ZEPHYR_BASE}/boards/xtensa/intel_adsp_cavs15/tools/cavs-fw-v15.py

if [ -z "$2" ]
  then
    echo "Signing using default key"
    west sign -d ${BUILD} -t rimage
elif [ -n "$3" ] && [ -n "$4" ]
  then
    echo "Signing with key " $key
    west sign -d ${BUILD} -t rimage -p $4 -D $3 -- -k $2 --no-manifest
fi

# Make the log output can be caught by non-root permission
ssh root@localhost chmod 666 /sys/bus/pci/devices/0000:00:0e.0/resource4

echo ${FLASHER} ${FIRMWARE}
ssh root@localhost "${FLASHER} ${FIRMWARE}"
