#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="security_bond_overwrite_allowed"
verbosity_level=2

central_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_$(guess_test_long_name)_prj_conf"
peripheral_exe="${central_exe}"

cd ${BSIM_OUT_PATH}/bin

Execute "$central_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -RealEncryption=1

Execute "$peripheral_exe" \
    -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -RealEncryption=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=60e6 $@

wait_for_background_jobs
