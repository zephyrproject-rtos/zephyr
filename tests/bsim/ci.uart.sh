#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This script runs the Babblesim CI UART tests.
# It can also be run locally.
# Note it will produce its output in ${ZEPHYR_BASE}/bsim_out/

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd ${ZEPHYR_BASE}

set -uex

echo "UART: Single device tests"
echo " nRF52833 & 5340:"
${ZEPHYR_BASE}/scripts/twister -T tests/drivers/uart/ --force-color --inline-logs -v -M \
  -p nrf52_bsim -p nrf5340bsim/nrf5340/cpuapp --fixture gpio_loopback \
  -- -uart0_loopback -uart1_loopback

echo " nRF54L15:"
${ZEPHYR_BASE}/scripts/twister -T tests/drivers/uart/ --force-color --inline-logs -v -M \
  -p nrf54l15bsim/nrf54l15/cpuapp --fixture gpio_loopback \
  -- -uart0_loopback -uart2_loopback
