#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This script runs the Babblesim CI networking tests.
# It can also be run locally.
# Note it will produce its output in ${ZEPHYR_BASE}/bsim_bt/

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd ${ZEPHYR_BASE}

set -uex

${ZEPHYR_BASE}/scripts/twister -p nrf52_bsim -T tests/bsim/net/ \
  --force-color --inline-logs -vv --fixture bsim_multi_test
