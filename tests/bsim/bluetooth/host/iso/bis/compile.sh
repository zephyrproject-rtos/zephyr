#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

if [ "${BOARD_TS}" == "nrf5340bsim_nrf5340_cpuapp" ]; then
    app=tests/bsim/bluetooth/host/iso/bis exe_name=bs_${BOARD_TS}_${app}_prj_conf \
      sysbuild=1 compile
else
    app=tests/bsim/bluetooth/host/iso/bis conf_overlay=controller.conf \
      exe_name=bs_${BOARD_TS}_${app}_prj_conf \
      sysbuild=1 compile
fi

wait_for_background_jobs
