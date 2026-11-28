#!/usr/bin/env bash
# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# PAST with the roles swapped: a Peripheral syncs to a periodic advertiser and
# sends PAST to the Central it is connected to, which then syncs to the PA.

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${BOARD_TS}_past_central_recipient"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=receive_past_central

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=broadcast_pa

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=send_past_peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=60e6 $@

wait_for_background_jobs
