#!/usr/bin/env bash
# Copyright (c) 2020-2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

SIMULATION_ID="bass_server_sync_client_rem"
VERBOSITY_LEVEL=2

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running BASS Server Sync Client Remove =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 \
  -testid=bap_scan_delegator_server_sync_client_rem -rs=24 -D=3

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 \
  -testid=bap_broadcast_assistant_server_sync_client_rem -rs=46 -D=3

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=bass_broadcaster -rs=69 -D=3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -D=3 \
  -sim_length=60e6 $@

wait_for_background_jobs
