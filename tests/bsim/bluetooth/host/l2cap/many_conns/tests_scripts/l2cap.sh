#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="l2cap_many_conns"

bsim_exe=./bs_${BOARD_TS}_tests_bsim_bluetooth_host_l2cap_many_conns_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -s=${simulation_id} -d=0 -testid=central -rs=42

Execute "${bsim_exe}" -s=${simulation_id} -d=1 -testid=peripheral -rs=1
Execute "${bsim_exe}" -s=${simulation_id} -d=2 -testid=peripheral -rs=2
Execute "${bsim_exe}" -s=${simulation_id} -d=3 -testid=peripheral -rs=3
Execute "${bsim_exe}" -s=${simulation_id} -d=4 -testid=peripheral -rs=4

Execute ./bs_2G4_phy_v1 -s=${simulation_id} -D=5 -sim_length=10e6 $@

wait_for_background_jobs
