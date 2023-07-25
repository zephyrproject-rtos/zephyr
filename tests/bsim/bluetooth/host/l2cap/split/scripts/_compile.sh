#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Path checks, etc
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

BOARD="${BOARD:-nrf52_bsim}"
dut_exe="bs_${BOARD}_tests_bsim_bluetooth_host_l2cap_split_dut_prj_conf"
tester_exe="bs_${BOARD}_tests_bsim_bluetooth_host_l2cap_split_tester_prj_conf"

# terminate running simulations (if any)
${BSIM_COMPONENTS_PATH}/common/stop_bsim.sh

west build -b ${BOARD} -d build_dut dut && \
    cp build_dut/zephyr/zephyr.exe "${BSIM_OUT_PATH}/bin/${dut_exe}" &&
west build -b ${BOARD} -d build_tester tester && \
    cp build_tester/zephyr/zephyr.exe "${BSIM_OUT_PATH}/bin/${tester_exe}"
