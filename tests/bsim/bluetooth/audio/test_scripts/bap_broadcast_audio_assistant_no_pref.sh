#!/usr/bin/env bash
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

SIMULATION_ID="${BOARD_TS}_bap_broadcast_audio_assistant_no_pref"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running BAP Broadcast audio assistant with NO_PREF =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -RealEncryption=1 \
  -testid=bap_scan_delegator_client_sync_no_pref -rs=24 -D=3

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -RealEncryption=1 \
  -testid=bap_broadcast_assistant_client_sync_no_pref -rs=46 -D=3

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -RealEncryption=1 \
  -testid=broadcast_source_encrypted -rs=69 -D=3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -D=3 \
  -sim_length=60e6 $@

wait_for_background_jobs
