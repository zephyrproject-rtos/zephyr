#!/usr/bin/env bash
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Smoketest for GAP Discovery

set -ue

COMMON_DIR="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common"

# Source utilities
source "$COMMON_DIR/common_utils.sh"

# Get test name from script's parent directory (two levels up)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_NAME="$(basename "$(dirname "$SCRIPT_DIR")")"

# Create gap_discovery directory if it doesn't exist
TEST_OUTPUT_DIR="${BT_CLASSIC_SIM}/${TEST_NAME}"
mkdir -p "$TEST_OUTPUT_DIR"

BUMBLE_CONTROLLERS_LOG="$TEST_OUTPUT_DIR/bumble_controllers.log"

# Initialize variables for cleanup
HCI_PORT_0=""
HCI_PORT_1=""
BUMBLE_CONTROLLER_PID=""

# Cleanup function to be called on exit
cleanup() {
    local exit_code=$?

    if [[ -f "$BUMBLE_CONTROLLERS_LOG" ]]; then
        cat "$BUMBLE_CONTROLLERS_LOG"
    fi

    # Stop bumble controllers if running
    if [[ -n "$BUMBLE_CONTROLLER_PID" ]]; then
        stop_bumble_controllers $BUMBLE_CONTROLLER_PID || true
    fi

    # Release TCP ports
    if [[ -n "$HCI_PORT_0" ]]; then
        release_tcp_port $HCI_PORT_0 || true
    fi

    if [[ -n "$HCI_PORT_1" ]]; then
        release_tcp_port $HCI_PORT_1 || true
    fi

    exit $exit_code
}

# Register cleanup function to run on exit
trap cleanup EXIT

HCI_PORT_0=$(allocate_tcp_port)
if [[ -z "$HCI_PORT_0" ]]; then
    exit 1
fi

HCI_PORT_1=$(allocate_tcp_port)
if [[ -z "$HCI_PORT_1" ]]; then
    exit 1
fi

BUMBLE_CONTROLLER_PID=$(start_bumble_controllers \
    "$BUMBLE_CONTROLLERS_LOG" \
    "$HCI_PORT_0@${BD_ADDR[0]}" "$HCI_PORT_1@${BD_ADDR[1]}")

if [[ -z "$BUMBLE_CONTROLLER_PID" ]]; then
    echo "Error: Failed to start bumble controllers" >&2
    exit 1
fi

test_case_name="bluetooth.classic.gap.discovery"
peripheral_exe="${BT_CLASSIC_BIN}/bt_classic_${BOARD_SANITIZED}_${test_case_name}.exe"
ARGS="--peer_bd_address=${BD_ADDR[1]} -test=gap_peripheral::*"

execute_async "$peripheral_exe" --bt-dev="127.0.0.1:$HCI_PORT_0" "$ARGS" || exit 1

test_case_name="bluetooth.classic.gap.discovery"
central_exe="${BT_CLASSIC_BIN}/bt_classic_${BOARD_SANITIZED}_${test_case_name}.exe"
ARGS="--peer_bd_address=${BD_ADDR[0]} -test=gap_central::*"

execute_async "$central_exe" --bt-dev="127.0.0.1:$HCI_PORT_1" "$ARGS" || exit 1

wait_for_async_jobs
