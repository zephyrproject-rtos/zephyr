#!/usr/bin/env bash
#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="cap_handover_central"
VERBOSITY_LEVEL=2
NR_OF_DEVICES=3
EXECUTE_TIMEOUT=240

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running CAP handover unicast to broadcast =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 \
  -testid=cap_handover_central \
  -RealEncryption=1 -rs=23 -D=${NR_OF_DEVICES}

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 \
  -testid=cap_handover_peripheral_unicast_to_broadcast \
  -RealEncryption=1 -rs=46 -D=${NR_OF_DEVICES}

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 \
  -testid=cap_handover_peripheral_unicast_to_broadcast \
  -RealEncryption=1 -rs=69 -D=${NR_OF_DEVICES}

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=${NR_OF_DEVICES} -sim_length=120e6 $@

wait_for_background_jobs
