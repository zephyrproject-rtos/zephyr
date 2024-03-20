#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="$(guess_test_long_name)_2"
verbosity_level=2
test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"
test_2_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_prj_2_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "$test_2_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=dut_2 -RealEncryption=1

Execute "$test_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=tester_2 -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
    -D=2 -sim_length=10e6 $@

wait_for_background_jobs
