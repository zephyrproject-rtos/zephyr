#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="bap_broadcast_audio_assistant_incorrect_code"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=180

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running BAP Broadcast audio assistant incorrect code =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -RealEncryption=1 \
  -testid=broadcast_sink_with_assistant_incorrect_code -rs=24 -D=3

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -RealEncryption=1 \
  -testid=bap_broadcast_assistant_client_sync_incorrect_code -rs=46 -D=3

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -RealEncryption=1 \
  -testid=broadcast_source_encrypted -rs=69 -D=3

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -D=3 \
  -sim_length=60e6 $@

wait_for_background_jobs
