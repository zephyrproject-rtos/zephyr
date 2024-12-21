#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="$(guess_test_long_name)"
test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_prj_conf"

simulation_id=${test_name}
verbosity_level=2
zephyr_log_level=3

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
  -D=2 -sim_length=100e6

Execute "${test_exe}" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 \
    -testid=peer -argstest log_level ${zephyr_log_level}

Execute "${test_exe}" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 \
    -testid=dut -argstest log_level ${zephyr_log_level}

wait_for_background_jobs
