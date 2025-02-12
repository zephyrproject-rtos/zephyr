#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="$(guess_test_long_name)"

simulation_id=${test_name}
verbosity_level=2

SIM_LEN_US=$((1 * 1000 * 1000))

dut_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_dut_prj_conf"
peer_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_peer_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "${dut_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -rs=420 -testid=dut
Execute "${peer_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -rs=69  -testid=peer

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=${SIM_LEN_US} $@

wait_for_background_jobs
