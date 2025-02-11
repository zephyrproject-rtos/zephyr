#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id="id_addr_update"

test_path="$(guess_test_long_name)"
central_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_path}_central_prj_conf"
peripheral_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_path}_peripheral_prj_conf"

cd ${BSIM_OUT_PATH}/bin

Execute "$central_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -RealEncryption=1

Execute "$peripheral_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -RealEncryption=1 -rs=200

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs
