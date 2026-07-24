#!/usr/bin/env bash
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Ensure this script is only loaded once
if [[ -n "${PARAMETER_UTILS_LOADED:-}" ]]; then
    return 0
fi
PARAMETER_UTILS_LOADED=1

# Default binary output directory
BT_CLASSIC_BIN="${BT_CLASSIC_BIN:-${ZEPHYR_BASE}/bt_classic_bin}"

BT_CLASSIC_SIM="${BT_CLASSIC_SIM:-${ZEPHYR_BASE}/bt_classic_sim}"

# Create output directory if it doesn't exist
mkdir -p "${BT_CLASSIC_BIN}"

mkdir -p "${BT_CLASSIC_SIM}"

# Static array for BD ADDRESS, starting from 00:00:01:00:00:01
BD_ADDR=(
    "00:00:01:00:00:01"
    "00:00:01:00:00:02"
    "00:00:01:00:00:03"
    "00:00:01:00:00:04"
    "00:00:01:00:00:05"
    "00:00:01:00:00:06"
    "00:00:01:00:00:07"
    "00:00:01:00:00:08"
    "00:00:01:00:00:09"
    "00:00:01:00:00:0a"
    "00:00:01:00:00:0b"
    "00:00:01:00:00:0c"
    "00:00:01:00:00:0d"
    "00:00:01:00:00:0e"
    "00:00:01:00:00:0f"
    "00:00:01:00:00:10"
)
