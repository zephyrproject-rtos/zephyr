#!/usr/bin/env bash
# Copyright (c) 2020-2022 Nordic Semiconductor ASA
# Copyright (c) 2026 Silicon Laboratories Inc.
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

SIMULATION_ID="${BOARD_TS}_bass_discover_retry"
VERBOSITY_LEVEL=2

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running BASS Discovery Retry =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 \
  -testid=bap_scan_delegator_discover_retry -RealEncryption=1 -D=2

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 \
  -testid=bap_broadcast_assistant_discover_retry -RealEncryption=1 -D=2

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -D=2 \
  -sim_length=10e6 $@

wait_for_background_jobs
