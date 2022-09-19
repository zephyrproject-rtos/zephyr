#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

# Read variable definitions output by _env.sh
source <("${bash_source_dir}/_env.sh")


: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be defined}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_bt_out}"
BOARD="${BOARD:-nrf52_bsim}"
BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"
INCR_BUILD=1
mkdir -p ${WORK_DIR}
source ${ZEPHYR_BASE}/tests/bluetooth/bsim_bt/compile.source
app="tests/bluetooth/bsim_bt/$test_name" compile
