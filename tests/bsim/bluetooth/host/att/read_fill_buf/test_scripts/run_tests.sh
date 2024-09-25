#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="read_fill_buf"
test_exe_d0="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_client_prj_conf"
test_exe_d1="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_server_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "$test_exe_d0" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=the_test \
    -RealEncryption=1

Execute "$test_exe_d1" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=the_test \
    -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs
