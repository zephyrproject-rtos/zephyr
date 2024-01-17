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

app=tests/bsim/bluetooth/host/security/bond_overwrite_allowed compile
app=tests/bsim/bluetooth/host/security/bond_overwrite_denied compile
app=tests/bsim/bluetooth/host/security/bond_per_connection compile
app=tests/bsim/bluetooth/host/security/ccc_update compile
app=tests/bsim/bluetooth/host/security/ccc_update conf_file=prj_2.conf compile
app=tests/bsim/bluetooth/host/security/id_addr_update/central compile
app=tests/bsim/bluetooth/host/security/id_addr_update/peripheral compile
app=tests/bsim/bluetooth/host/security/security_changed_callback compile

wait_for_background_jobs
