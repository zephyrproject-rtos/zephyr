#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0
set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="$(guess_test_long_name)"
simulation_id="${BOARD_TS}_${test_name}_invalid_handles"

verbosity_level=2

SIM_LEN_US=$((5 * 1000 * 1000))

test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=${SIM_LEN_US} $@

Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 \
    -testid=invalid_handles_server
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 \
    -testid=invalid_handles_client

wait_for_background_jobs
