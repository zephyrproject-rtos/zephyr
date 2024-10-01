#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

dut_exe="bs_${BOARD_TS}_$(guess_test_long_name)_dut_prj_conf"
tester_exe="bs_${BOARD_TS}_$(guess_test_long_name)_tester_prj_conf"

simulation_id="l2cap_split"
verbosity_level=2
sim_length_us=30e6

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=2 -sim_length=${sim_length_us} $@
Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=test_0 -RealEncryption=1
Execute "./$tester_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=test_0 -RealEncryption=1

wait_for_background_jobs
