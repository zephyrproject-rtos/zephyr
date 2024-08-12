#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

run_in_background \
    ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/security/bond_overwrite_allowed/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/security/bond_overwrite_denied/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/security/bond_per_connection/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/security/ccc_update/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/security/id_addr_update/compile.sh
run_in_background \
    ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/security/security_changed_callback/compile.sh

wait_for_background_jobs
