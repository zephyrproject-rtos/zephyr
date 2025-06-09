#!/usr/bin/env bash
# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# EATT test
simulation_id="l2cap_stress_early_disconnect"
verbosity_level=2
EXECUTE_TIMEOUT=240

bsim_exe=./bs_${BOARD_TS}_tests_bsim_bluetooth_host_l2cap_stress_prj_conf_overlay-early-disc_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=43

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 \
	-testid=peripheral_early_disconnect -rs=42
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=2 \
	-testid=peripheral_early_disconnect -rs=10
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=3 \
	-testid=peripheral_early_disconnect -rs=23
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=4 \
	-testid=peripheral_early_disconnect -rs=7884
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=5 \
	-testid=peripheral_early_disconnect -rs=230
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=6 \
	-testid=peripheral_early_disconnect -rs=9

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=7 -sim_length=400e6 $@

wait_for_background_jobs
