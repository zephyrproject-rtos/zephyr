#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# A central initiates with a finite scan window and a peripheral advertises
# around the end of that window: every connection that the central reports
# has to be one that the peripheral has as well.
simulation_id="${BOARD_TS}_conn_scan_window_split"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_conn_prj_split_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=peripheral_scan_window

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_conn_prj_split_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=central_scan_window

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=30e6 $@

wait_for_background_jobs
