#!/usr/bin/env bash
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

SIMULATION_ID="${BOARD_TS}_bap_broadcast_audio_assistant_colocated_past"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=240

cd ${BSIM_OUT_PATH}/bin

printf "\n\n===== Running BAP Broadcast Audio Assistant with Colocated Source with PAST ======\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 \
  -testid=broadcast_sink_with_assistant -RealEncryption=1 -rs=24 -D=2 -argstest past 1

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 \
  -testid=bap_broadcast_assistant_colocated -RealEncryption=1 -rs=46 -D=2

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -D=2 -sim_length=60e6 $@

wait_for_background_jobs
