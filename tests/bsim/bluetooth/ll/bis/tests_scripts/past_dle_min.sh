#!/usr/bin/env bash
# Copyright (c) 2024 Demant A/S
# Copyright (c) 2026 Kent Hauser
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# PAST at the minimum data length (no Data Length Extension): same scenario
# as past.sh, but the link never raises connMaxRxOctets, so
# LL_PERIODIC_SYNC_IND (35 octets) exceeds a single PDU.
# Basic PAST Test: Broadcaster is broadcasting Periodic Advertisements (PA).
# A central device synchronizes to the PA after it is synced,
# it will connect to the peripheral device.
# After connection is established the Central send a Periodic Sync Transfer (PAST)
# to the peripheral, which then synchronizes to the PA.
simulation_id="${BOARD_TS}_past_dle_min"
verbosity_level=2

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf_overlay-past_dle_min_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=receive_past

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf_overlay-past_dle_min_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=broadcast_pa

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf_overlay-past_dle_min_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=send_past

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=60e6 $@

wait_for_background_jobs
