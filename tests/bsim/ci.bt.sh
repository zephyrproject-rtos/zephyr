#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This script runs the Babblesim CI BT tests.
# It can also be run locally.
# Note it will produce its output in ${ZEPHYR_BASE}/bsim_bt/

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd ${ZEPHYR_BASE}

set -uex

# nrf52_bsim set:
nice tests/bsim/bluetooth/compile.sh

RESULTS_FILE=${ZEPHYR_BASE}/bsim_out/bsim_results.bt.52.xml \
TESTS_FILE=tests/bsim/bluetooth/tests.nrf52bsim.txt \
tests/bsim/run_parallel.sh

# nrf5340bsim/nrf5340/cpunet set:
nice tests/bsim/bluetooth/compile.nrf5340bsim_nrf5340_cpunet.sh

BOARD=nrf5340bsim/nrf5340/cpunet \
RESULTS_FILE=${ZEPHYR_BASE}/bsim_out/bsim_results.bt.53_cpunet.xml \
TESTS_FILE=tests/bsim/bluetooth/tests.nrf5340bsim_nrf5340_cpunet.txt \
tests/bsim/run_parallel.sh

# nrf5340 split stack set:
nice tests/bsim/bluetooth/compile.nrf5340bsim_nrf5340_cpuapp.sh

BOARD=nrf5340bsim/nrf5340/cpuapp \
RESULTS_FILE=${ZEPHYR_BASE}/bsim_out/bsim_results.bt.53_cpuapp.xml \
TESTS_FILE=tests/bsim/bluetooth/tests.nrf5340bsim_nrf5340_cpuapp.txt \
tests/bsim/run_parallel.sh

# nrf54l15bsim/nrf54l15/cpuapp set:
# We provisionally disable the nrf54l15 BT tests in CI due to instability issues in the
# controller for this platform. See https://github.com/zephyrproject-rtos/zephyr/issues/74635
# This should be reverted once the underlaying issue is fixed
#nice tests/bsim/bluetooth/compile.nrf54l15bsim_nrf54l15_cpuapp.sh

#BOARD=nrf54l15bsim/nrf54l15/cpuapp \
#RESULTS_FILE=${ZEPHYR_BASE}/bsim_out/bsim_results.bt.54l15_cpuapp.xml \
#TESTS_FILE=tests/bsim/bluetooth/tests.nrf54l15bsim_nrf54l15_cpuapp.txt \
#tests/bsim/run_parallel.sh
