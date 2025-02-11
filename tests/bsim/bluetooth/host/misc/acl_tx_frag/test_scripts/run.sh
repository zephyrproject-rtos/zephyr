#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="$(guess_test_long_name)"
simulation_id=${test_name}
verbosity_level=2

# sixty-second (maximum) sim time.
# The test will exit simulation as soon as it has passed.
SIM_LEN_US=$((60 * 1000 * 1000))

test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -rs=420 -testid=dut \
		-argstest log_level 3
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -rs=69  -testid=peer \
		-argstest log_level 3 >/dev/null

Execute ./bs_2G4_phy_v1 -defmodem=BLE_simple \
    -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=${SIM_LEN_US} $@

wait_for_background_jobs
