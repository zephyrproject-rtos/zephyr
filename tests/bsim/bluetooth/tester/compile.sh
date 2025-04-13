#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue

source ${ZEPHYR_BASE}/tests/bsim/compile.source

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

# We only want to build the tests/bsim/bluetooth/tester/ application for the nrf52_bsim/native board
# as we do not gain anything from running this application for the nrf5340bsim_nrf5340_cpuapp and
# it is not worth the added complexity to run it with anything but nrf52_bsim/native
west twister -T ${ZEPHYR_BASE}/tests/bsim/bluetooth/tester/

west twister -p ${BOARD} -T ${ZEPHYR_BASE}/tests/bluetooth/tester/

wait_for_background_jobs
