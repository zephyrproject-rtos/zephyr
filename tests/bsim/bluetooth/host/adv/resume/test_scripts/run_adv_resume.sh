#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="$(guess_test_long_name)"
verbosity_level=2
test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "$test_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=dut -RealEncryption=1

Execute "$test_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=tester_central -RealEncryption=1

Execute "$test_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=tester_peripheral -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=3 -sim_length=10e6 $@

# Uncomment (and comment the other peripheral line) to run DUT under a debugger
# gdb --args "$test_exe" \
#     -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -RealEncryption=1

wait_for_background_jobs
