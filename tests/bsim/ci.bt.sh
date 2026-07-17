#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This script runs the Babblesim CI BT tests.
# It can also be run locally.
# Note it will produce its output in ${ZEPHYR_BASE}/bsim_bt/

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd ${ZEPHYR_BASE}

set -uex

TWISTER_OPTIONS="-vv --fixture bsim_multi_test --no-clean --force-color --inline-logs"

${ZEPHYR_BASE}/scripts/twister ${TWISTER_OPTIONS} -T tests/bsim/bluetooth/
