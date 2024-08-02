#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_lowres.conf compile
app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_collision.conf compile
app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_multiple_conn.conf compile
app=tests/bsim/bluetooth/host/att/eatt conf_file=prj_autoconnect.conf compile
app=tests/bsim/bluetooth/host/att/eatt_notif conf_file=prj.conf compile
app=tests/bsim/bluetooth/host/att/mtu_update compile
app=tests/bsim/bluetooth/host/att/read_fill_buf/client compile
app=tests/bsim/bluetooth/host/att/read_fill_buf/server compile
app=tests/bsim/bluetooth/host/att/retry_on_sec_err/client compile
app=tests/bsim/bluetooth/host/att/retry_on_sec_err/server compile
app=tests/bsim/bluetooth/host/att/sequential/dut compile
app=tests/bsim/bluetooth/host/att/sequential/tester compile
app=tests/bsim/bluetooth/host/att/pipeline/dut compile
app=tests/bsim/bluetooth/host/att/pipeline/dut \
    conf_file='prj.conf;rx_tx_prio_invert.extra.conf' compile
app=tests/bsim/bluetooth/host/att/pipeline/tester compile
app=tests/bsim/bluetooth/host/att/long_read compile
app=tests/bsim/bluetooth/host/att/open_close compile

wait_for_background_jobs
