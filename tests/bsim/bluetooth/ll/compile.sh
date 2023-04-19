#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests

#set -x #uncomment this line for debugging
set -ue
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_out}"

BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"

mkdir -p ${WORK_DIR}

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/ll/advx compile
app=tests/bsim/bluetooth/ll/advx \
  conf_overlay=overlay-ticker_expire_info.conf compile

app=tests/bsim/bluetooth/ll/conn conf_file=prj_split.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_privacy.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_low_lat.conf compile

app=tests/bsim/bluetooth/ll/iso compile
app=tests/bsim/bluetooth/ll/iso \
  conf_overlay=overlay-ticker_expire_info.conf compile
app=tests/bsim/bluetooth/ll/iso conf_file=prj_vs_dp.conf compile

app=tests/bsim/bluetooth/ll/edtt/hci_test_app \
  conf_file=prj_dut_llcp.conf compile
app=tests/bsim/bluetooth/ll/edtt/hci_test_app \
  conf_file=prj_tst_llcp.conf compile
app=tests/bsim/bluetooth/ll/edtt/gatt_test_app \
  conf_file=prj_llcp.conf compile

app=tests/bsim/bluetooth/ll/multiple_id compile
app=tests/bsim/bluetooth/ll/throughput compile

wait_for_background_jobs
