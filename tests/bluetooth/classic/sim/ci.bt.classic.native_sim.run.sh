#!/usr/bin/env bash
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

#set -x #uncomment this line for debugging

set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

COMMON_DIR="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common"

export BOARD="native_sim"

# Source utilities
source "$COMMON_DIR/common_utils.sh"

CONFIG="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/tests.native_sim.txt"
TEST_DIR="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/"

SCRIPTS=$(run_scripts "${CONFIG}" "${TEST_DIR}")
RESULTS_FILE="${BT_CLASSIC_SIM}/${BOARD_SANITIZED}/TestResults.xml"
run_parallel "${SCRIPTS}"
