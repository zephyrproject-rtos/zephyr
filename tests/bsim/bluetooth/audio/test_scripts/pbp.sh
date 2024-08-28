#!/usr/bin/env bash
#
# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0

VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=100

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Public Broadcaster test =========\n\n"

SIMULATION_ID="pbp_broadcaster"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=public_broadcast_source \
  -RealEncryption=1 -rs=27 -D=2

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=public_broadcast_sink \
  -RealEncryption=1 -rs=27 -D=2

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=2 -sim_length=100e6 $@

wait_for_background_jobs
