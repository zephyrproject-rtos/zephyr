#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the Bluetooth bsim tests on the nrf5340_cpunet

#set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

export BOARD="${BOARD:-nrf5340bsim/nrf5340/cpunet}"

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

${ZEPHYR_BASE}/tests/bsim/bluetooth/ll/compile.sh
${ZEPHYR_BASE}/tests/bsim/bluetooth/host/compile.sh

wait_for_background_jobs
