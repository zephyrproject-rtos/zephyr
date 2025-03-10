#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue

source ${ZEPHYR_BASE}/tests/bsim/compile.source

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

west twister -T ${ZEPHYR_BASE}/tests/bsim/bluetooth/tester/

app=tests/bluetooth/tester \
    exe_name=bs_${BOARD_TS}_${app}_prj_conf sysbuild=1 compile

wait_for_background_jobs
