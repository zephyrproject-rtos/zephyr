#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

verbosity_level=2
EXECUTE_TIMEOUT=20

cd ${BSIM_OUT_PATH}/bin

simulation_id=$(basename $BASH_SOURCE)
bsim_exe=./bs_nrf52_bsim_tests_bsim_bluetooth_host_l2cap_credits_prj_conf

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=420
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -rs=100

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=30e6 $@

wait_for_background_jobs

simulation_id=$(basename $BASH_SOURCE)_ecred
bsim_exe=./bs_nrf52_bsim_tests_bsim_bluetooth_host_l2cap_credits_prj_ecred_conf

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=420
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -rs=100

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=30e6 $@

wait_for_background_jobs
