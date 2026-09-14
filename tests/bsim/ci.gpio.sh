#!/usr/bin/env bash
# Copyright (c) 2026 Demant A/S
# SPDX-License-Identifier: Apache-2.0

# This script runs the Babblesim CI GPIO tests.
# It can also be run locally.

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd ${ZEPHYR_BASE}

set -uex

${ZEPHYR_BASE}/scripts/twister -p nrf52_bsim -T tests/bsim/drivers/gpio/ \
  --force-color --inline-logs -vv --fixture bsim_multi_test -O twister-out-gpio
