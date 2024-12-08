#!/usr/bin/env bash
# Copyright 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=samples/bluetooth/peripheral_hr \
  sysbuild=1 \
  compile
app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
  sysbuild=1 \
  extra_conf_file=${ZEPHYR_BASE}/samples/bluetooth/central_hr/prj.conf \
  compile
app=samples/bluetooth/peripheral_hr \
  sysbuild=1 \
  conf_overlay=overlay-extended.conf \
  compile
app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
  sysbuild=1 \
  extra_conf_file=${ZEPHYR_BASE}/samples/bluetooth/central_hr/prj.conf \
  conf_overlay=${ZEPHYR_BASE}/samples/bluetooth/central_hr/overlay-extended.conf \
  compile
app=samples/bluetooth/peripheral_hr \
  sysbuild=1 \
  conf_overlay=overlay-phy_coded.conf \
  compile
app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
  sysbuild=1 \
  extra_conf_file=${ZEPHYR_BASE}/samples/bluetooth/central_hr/prj.conf \
  conf_overlay=${ZEPHYR_BASE}/samples/bluetooth/central_hr/overlay-phy_coded.conf \
  compile
if [ ${BOARD} == "nrf52_bsim/native" ]; then
  app=tests/bsim/bluetooth/samples/battery_service \
    conf_file=prj.conf \
    compile
fi

wait_for_background_jobs
