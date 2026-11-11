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
  exe_name=bs_${BOARD_TS}_${app}_prj_conf \
  compile
app=samples/bluetooth/peripheral_hr \
  sysbuild=1 \
  extra_conf_file=extra-extended.conf \
  exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-extended_conf \
  compile
app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
  sysbuild=1 \
  sample=${ZEPHYR_BASE}/samples/bluetooth/central_hr \
  extra_conf_file="${sample}/prj.conf;${sample}/extra-extended.conf" \
  exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-extended_conf \
  compile
app=samples/bluetooth/peripheral_hr \
  sysbuild=1 \
  extra_conf_file=extra-phy_coded.conf \
  exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-phy_coded_conf \
  compile
app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
  sysbuild=1 \
  sample=${ZEPHYR_BASE}/samples/bluetooth/central_hr \
  extra_conf_file="${sample}/prj.conf;${sample}/extra-phy_coded.conf" \
  exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-phy_coded_conf \
  compile
app=samples/bluetooth/peripheral_hr \
  sysbuild=1 \
  extra_conf_file=extra-static_callbacks.conf \
  exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-static_callbacks_conf \
  compile
app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
  sysbuild=1 \
  sample=${ZEPHYR_BASE}/samples/bluetooth/central_hr \
  extra_conf_file="${sample}/prj.conf;${sample}/extra-static_callbacks.conf" \
  exe_name=bs_${BOARD_TS}_${app}_prj_conf_extra-static_callbacks_conf \
  compile
if [ ${BOARD} == "nrf52_bsim/native" ]; then
  app=tests/bsim/bluetooth/samples/battery_service \
    conf_file=prj.conf \
    compile
fi
app=samples/bluetooth/peripheral_gap_svc \
  sysbuild=1 \
  compile
app=tests/bsim/bluetooth/samples/peripheral_gap_svc \
  sysbuild=1 \
  compile

wait_for_background_jobs
