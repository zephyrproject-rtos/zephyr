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
${ZEPHYR_BASE}/scripts/twister -T tests/drivers/uart/ --force-color --inline-logs -v -M \
  -p nrf52_bsim -p nrf5340bsim/nrf5340/cpuapp --fixture gpio_loopback \
  -- -uart0_loopback -uart1_loopback

echo "UART: Multi device tests"
WORK_DIR=${ZEPHYR_BASE}/bsim_uart nice tests/bsim/drivers/uart/compile.sh
RESULTS_FILE=${ZEPHYR_BASE}/bsim_out/bsim_results.uart.52.xml \
SEARCH_PATH=tests/bsim/drivers/uart/ tests/bsim/run_parallel.sh
