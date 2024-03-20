#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
simulation_id=$(guess_test_long_name)
bsim_exe=./bs_${BOARD}_$(guess_test_long_name)_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=420
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -rs=100

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=30e6 $@

wait_for_background_jobs
