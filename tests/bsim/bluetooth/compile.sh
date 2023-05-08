#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the Bluetooth bsim tests

#set -x #uncomment this line for debugging
set -ue
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_out}"

BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"

mkdir -p ${WORK_DIR}

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/audio/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/ll/compile.sh
run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/mesh/compile.sh

wait_for_background_jobs
