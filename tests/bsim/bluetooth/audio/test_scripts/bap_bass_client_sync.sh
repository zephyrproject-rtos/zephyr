#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="bass_client_sync"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=120

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running BASS Client Sync =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=bap_scan_delegator_client_sync \
  -RealEncryption=1 -rs=24 -D=3

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 \
  -testid=bap_broadcast_assistant_client_sync -RealEncryption=1 -rs=46 -D=3 \
  -start_offset=2e3

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=bass_broadcaster \
  -RealEncryption=1 -rs=69 -D=3 -start_offset=2e3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -D=3 \
  -sim_length=60e6 $@

wait_for_background_jobs
