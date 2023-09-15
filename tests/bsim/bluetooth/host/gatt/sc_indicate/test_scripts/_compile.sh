#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu

# Terminate running simulations (if any)
${BSIM_COMPONENTS_PATH}/common/stop_bsim.sh

test_name='sc_indicate'

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"
bsim_bin="${BSIM_OUT_PATH}/bin"
BOARD="${BOARD:-nrf52_bsim}"
test_exe="${bsim_bin}/bs_${BOARD}_tests_bsim_bluetooth_host_gatt_${test_name}_prj_conf"

west build -b nrf52_bsim -d build && \
        cp -v build/zephyr/zephyr.exe "${test_exe}"
