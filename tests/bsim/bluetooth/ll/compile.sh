#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

${ZEPHYR_BASE}/tests/bsim/bluetooth/ll/cis/compile.sh

app=tests/bsim/bluetooth/ll/advx compile
app=tests/bsim/bluetooth/ll/advx \
  conf_overlay=overlay-ticker_expire_info.conf compile
app=tests/bsim/bluetooth/ll/advx conf_overlay=overlay-scan_aux_use_chains.conf compile

app=tests/bsim/bluetooth/ll/conn conf_file=prj_split.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_1ms.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_tx_defer.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_privacy.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_low_lat.conf compile
app=tests/bsim/bluetooth/ll/conn conf_file=prj_split_single_timer.conf compile

app=tests/bsim/bluetooth/ll/bis conf_overlay=overlay-sequential.conf compile
app=tests/bsim/bluetooth/ll/bis conf_overlay=overlay-interleaved.conf  compile
app=tests/bsim/bluetooth/ll/bis conf_overlay=overlay-ll_interface.conf compile
app=tests/bsim/bluetooth/ll/bis conf_overlay=overlay-ticker_expire_info.conf compile
app=tests/bsim/bluetooth/ll/bis conf_overlay=overlay-scan_aux_use_chains.conf compile
app=tests/bsim/bluetooth/ll/bis conf_file=prj_vs_dp.conf compile
app=tests/bsim/bluetooth/ll/bis conf_file=prj_past.conf compile

app=tests/bsim/bluetooth/ll/edtt/hci_test_app \
  conf_file=prj_dut_llcp.conf compile
app=tests/bsim/bluetooth/ll/edtt/hci_test_app \
  conf_file=prj_tst_llcp.conf compile
app=tests/bsim/bluetooth/ll/edtt/gatt_test_app \
  conf_file=prj_llcp.conf compile

app=tests/bsim/bluetooth/ll/multiple_id compile
app=tests/bsim/bluetooth/ll/throughput compile

wait_for_background_jobs
