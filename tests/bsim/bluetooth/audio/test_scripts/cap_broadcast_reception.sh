#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="cap_broadcast_reception"
VERBOSITY_LEVEL=2
NR_OF_DEVICES=3
EXECUTE_TIMEOUT=180

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running CAP commander broadcast reception start and stop test =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=cap_commander_broadcast_reception \
  -rs=46 -D=${NR_OF_DEVICES}

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=broadcast_source \
  -rs=23 -D=${NR_OF_DEVICES}

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=cap_acceptor_broadcast_reception \
  -rs=69 -D=${NR_OF_DEVICES}

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=${NR_OF_DEVICES} -sim_length=60e6 $@



wait_for_background_jobs
