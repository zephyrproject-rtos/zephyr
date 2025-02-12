#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/host/l2cap/many_conns compile
app=tests/bsim/bluetooth/host/l2cap/general compile
app=tests/bsim/bluetooth/host/l2cap/userdata compile
app=tests/bsim/bluetooth/host/l2cap/stress compile
app=tests/bsim/bluetooth/host/l2cap/stress conf_file=prj_nofrag.conf compile
app=tests/bsim/bluetooth/host/l2cap/stress conf_file=prj_syswq.conf compile
app=tests/bsim/bluetooth/host/l2cap/split/dut compile
app=tests/bsim/bluetooth/host/l2cap/split/tester compile
app=tests/bsim/bluetooth/host/l2cap/reassembly/dut compile
app=tests/bsim/bluetooth/host/l2cap/reassembly/peer compile
app=tests/bsim/bluetooth/host/l2cap/ecred/dut compile
app=tests/bsim/bluetooth/host/l2cap/ecred/peer compile
app=tests/bsim/bluetooth/host/l2cap/credits compile
app=tests/bsim/bluetooth/host/l2cap/credits conf_file=prj_ecred.conf compile
app=tests/bsim/bluetooth/host/l2cap/credits_seg_recv compile
app=tests/bsim/bluetooth/host/l2cap/credits_seg_recv conf_file=prj_ecred.conf compile
app=tests/bsim/bluetooth/host/l2cap/send_on_connect compile
app=tests/bsim/bluetooth/host/l2cap/send_on_connect conf_file=prj_ecred.conf compile

wait_for_background_jobs
