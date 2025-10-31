#!/usr/bin/env bash
# Copyright (c) 2025 Embeint Pty Ltd
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="hci_vs"
verbosity_level=2

bsim_exe=./bs_${BOARD_TS}_tests_bsim_bluetooth_host_misc_hci_vs_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=write_bdaddr -rs=1

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=1 -sim_length=30e6 $@

wait_for_background_jobs
