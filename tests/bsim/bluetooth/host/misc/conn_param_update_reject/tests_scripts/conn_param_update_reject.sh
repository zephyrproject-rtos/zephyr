#!/usr/bin/env bash
# Copyright (c) 2025 Sharon Lin
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Connection parameter update rejection: a peripheral connects to a central and
# requests a connection parameter update. The central rejects every request, and
# the peripheral verifies that the le_param_update_rejected callback is invoked
# with a non-zero HCI error code.
simulation_id="${BOARD_TS}_conn_param_update_reject"
verbosity_level=2

bsim_exe=./bs_${BOARD_TS}_tests_bsim_bluetooth_host_misc_conn_param_update_reject_prj_conf

cd ${BSIM_OUT_PATH}/bin

Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -rs=420
Execute "${bsim_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -rs=100

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=30e6 $@

wait_for_background_jobs
