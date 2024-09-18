#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

${ZEPHYR_BASE}/tests/bsim/bluetooth/host/adv/compile.sh
app=tests/bsim/bluetooth/host/central compile
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/att/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/gatt/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/l2cap/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/security/compile.sh

app=tests/bsim/bluetooth/host/iso/cis compile
app=tests/bsim/bluetooth/host/iso/bis compile
app=tests/bsim/bluetooth/host/iso/frag compile
app=tests/bsim/bluetooth/host/iso/frag_2 compile

app=tests/bsim/bluetooth/host/misc/disable compile
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/misc/disconnect/compile.sh
app=tests/bsim/bluetooth/host/misc/conn_stress/central compile
app=tests/bsim/bluetooth/host/misc/conn_stress/peripheral compile
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/misc/hfc/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/misc/hfc_multilink/compile.sh
app=tests/bsim/bluetooth/host/misc/unregister_conn_cb compile
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/misc/sample_test/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/misc/acl_tx_frag/compile.sh

run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/privacy/central/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/privacy/peripheral/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/privacy/device/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/privacy/legacy/compile.sh

run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/id/settings/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/scan/start_stop/compile.sh

wait_for_background_jobs
