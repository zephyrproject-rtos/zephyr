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

${ZEPHYR_BASE}/tests/bsim/bluetooth/host/adv/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/att/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/gatt/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/l2cap/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/security/compile.sh

app=tests/bsim/bluetooth/host/iso/cis compile

app=tests/bsim/bluetooth/host/misc/disable compile
app=tests/bsim/bluetooth/host/misc/disconnect/dut compile
app=tests/bsim/bluetooth/host/misc/disconnect/tester compile
app=tests/bsim/bluetooth/host/misc/conn_stress/central compile
app=tests/bsim/bluetooth/host/misc/conn_stress/peripheral compile
app=tests/bsim/bluetooth/host/misc/unregister_conn_cb compile

app=tests/bsim/bluetooth/host/privacy/central compile
app=tests/bsim/bluetooth/host/privacy/peripheral compile
app=tests/bsim/bluetooth/host/privacy/peripheral conf_file=prj_rpa_sharing.conf compile
app=tests/bsim/bluetooth/host/privacy/device compile
app=tests/bsim/bluetooth/host/privacy/legacy compile

app=tests/bsim/bluetooth/host/id/settings compile

wait_for_background_jobs
