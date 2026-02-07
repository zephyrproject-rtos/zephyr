#!/usr/bin/env bash
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_NAME="$(basename "${SCRIPT_DIR}")"

COMMON_DIR="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common"

# Source utilities
source "$COMMON_DIR/common_utils.sh"

BUILD_TEMP_DIR=$(create_temp_dir "${TEST_NAME}_build")

# Build
west twister -p "${BOARD}" -T "${SCRIPT_DIR}" -O "${BUILD_TEMP_DIR}" --build-only
extra_twister_out "${BUILD_TEMP_DIR}" "${TEST_NAME}"
