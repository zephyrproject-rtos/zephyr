#!/usr/bin/env bash
#
# Copyright (c) 2026 Demant A/S
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

SIMULATION_ID="${BOARD_TS}_bap_broadcast_audio_assistant_collocated_past_privacy"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=240

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running BAP Broadcast Audio Assistant PAST "
printf "with Privacy (Collocated Source+Assistant) =========\n\n"

# Device 0: Broadcast sink with assistant support (PAST)
Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_privacy_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 \
  -testid=broadcast_sink_with_collocated_assistant_past -RealEncryption=1 -rs=47 -D=2

# Device 1: Collocated broadcast source + broadcast assistant (PAST)
Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_privacy_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 \
  -testid=bap_broadcast_source_assistant_for_broadcast_sink_past -RealEncryption=1 -rs=53 -D=2

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -D=2 -sim_length=60e6 $@

wait_for_background_jobs
