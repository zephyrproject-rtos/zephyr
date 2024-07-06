#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the Bluetooth bsim tests on the
# nrf54l15bsim/nrf54l15/cpuapp

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

export BOARD="${BOARD:-nrf54l15bsim/nrf54l15/cpuapp}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=tests/bsim/bluetooth/ll/multiple_id compile

run_in_background ${ZEPHYR_BASE}/tests/bsim/bluetooth/samples/compile.sh

wait_for_background_jobs
