#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

BOARD="${BOARD:-nrf52_bsim}"
dut_exe="bs_${BOARD}_tests_bsim_bluetooth_host_l2cap_split_dut_prj_conf"
tester_exe="bs_${BOARD}_tests_bsim_bluetooth_host_l2cap_split_tester_prj_conf"

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="l2cap_split"
simulation_id="${test_name}"
verbosity_level=2
EXECUTE_TIMEOUT=30
sim_length_us=30e6

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=2 -sim_length=${sim_length_us} $@
Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=test_0 -RealEncryption=1
Execute "./$tester_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=test_0 -RealEncryption=1

wait_for_background_jobs
