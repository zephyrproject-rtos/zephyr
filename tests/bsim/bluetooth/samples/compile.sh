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
  compile
app=samples/bluetooth/peripheral_hr \
  sysbuild=1 \
  file_suffix=extended \
  compile
app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
  sysbuild=1 \
  file_suffix=extended \
  compile
app=samples/bluetooth/peripheral_hr \
  sysbuild=1 \
  file_suffix=phy_coded \
  compile
app=tests/bsim/bluetooth/samples/central_hr_peripheral_hr \
  sysbuild=1 \
  file_suffix=phy_coded \
  compile

wait_for_background_jobs
