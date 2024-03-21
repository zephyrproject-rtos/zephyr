#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Path checks, etc
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

BOARD="${BOARD:-nrf52_bsim}"
dut_exe="bs_${BOARD}_tests_bsim_bluetooth_host_att_pipeline_dut_prj_conf"
tester_exe="bs_${BOARD}_tests_bsim_bluetooth_host_att_pipeline_tester_prj_conf"

# terminate running simulations (if any)
${BSIM_COMPONENTS_PATH}/common/stop_bsim.sh

west build -b ${BOARD} -d build_a dut && \
    cp build_a/zephyr/zephyr.exe "${BSIM_OUT_PATH}/bin/${dut_exe}" &&

# b stands for bad
west build -b ${BOARD} -d build_b tester && \
    cp build_b/zephyr/zephyr.exe "${BSIM_OUT_PATH}/bin/${tester_exe}"
