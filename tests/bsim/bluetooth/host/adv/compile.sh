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

app=tests/bsim/bluetooth/host/adv/resume compile
app=tests/bsim/bluetooth/host/adv/resume conf_file=prj_2.conf compile
app=tests/bsim/bluetooth/host/adv/chain compile
app=tests/bsim/bluetooth/host/adv/extended conf_file=prj_advertiser.conf compile
app=tests/bsim/bluetooth/host/adv/extended conf_file=prj_scanner.conf compile
app=tests/bsim/bluetooth/host/adv/periodic compile
app=tests/bsim/bluetooth/host/adv/periodic conf_file=prj_long_data.conf compile
app=tests/bsim/bluetooth/host/adv/encrypted/css_sample_data compile
app=tests/bsim/bluetooth/host/adv/encrypted/ead_sample compile

wait_for_background_jobs
