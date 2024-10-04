#!/usr/bin/env bash
# Copyright (c) 2024 Demant A/S
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Basic PAST Test: Broadcaster is broadcasting Periodic Advertisements (PA).
# A central device syncronizes to the PA after it is synced,
# it will connect to the peripheral device.
# After connection is established the Central send a Periodic Sync Transfer (PAST)
# to the peripheral, which then synchronizes to the PA.
simulation_id="past_basic"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=receive_past

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=broadcast_pa

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=send_past

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=60e6 $@

wait_for_background_jobs
