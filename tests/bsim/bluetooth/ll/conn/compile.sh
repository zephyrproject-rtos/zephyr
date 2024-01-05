#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_out}"

BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"

mkdir -p ${WORK_DIR}

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/ll/conn conf_file=prj_split.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_privacy.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_low_lat.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_single_timer.conf compile

wait_for_background_jobs
