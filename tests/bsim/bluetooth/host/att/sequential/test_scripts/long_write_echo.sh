#!/usr/bin/env bash
# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_att_long_write_echo"
verbosity_level=2

dut_exe="bs_${BOARD_TS}_$(guess_test_long_name)_dut_prj_conf"
tester_exe="bs_${BOARD_TS}_$(guess_test_long_name)_tester_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=2 -sim_length=10e6 $@

Execute "./$tester_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=tester_long_write_echo

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=dut_long_write_echo

wait_for_background_jobs
