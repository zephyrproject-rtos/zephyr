#!/usr/bin/env bash
# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# PAST Set Info Transfer before extended advertising is started is rejected;
# the controller must not crash, and a transfer after starting must succeed.
simulation_id="${BOARD_TS}_past_send_no_ext_adv"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=receive_past

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=broadcast_past_sender_no_ext_adv

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
