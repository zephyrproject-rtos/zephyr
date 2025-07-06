#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="$(guess_test_long_name)"

simulation_id=${test_name}
verbosity_level=2

# Simulated test runtime (as seen by Zephyr and the PHY)
# The test will be terminated much earlier if it passes
SIM_LEN_US=$((10 * 1000 * 1000))

tester_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_tester_prj_conf"
dut_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_dut_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=4 -sim_length=${SIM_LEN_US} $@

Execute "${dut_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -rs=420 -testid=dut

Execute "${tester_exe}" -s=${simulation_id} -d=1 -rs=100 -testid=tester
Execute "${tester_exe}" -s=${simulation_id} -d=2 -rs=200 -testid=tester
Execute "${tester_exe}" -s=${simulation_id} -d=3 -rs=300 -testid=tester

wait_for_background_jobs
