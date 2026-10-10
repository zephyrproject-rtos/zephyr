#!/usr/bin/env bash
#
# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

SIMULATION_ID="${BOARD_TS}_pacs_notify_retry"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=200

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running PACS Notify Retry test =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=pacs_notify_retry_server \
  -RealEncryption=1 -D=2

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=pacs_notify_retry_client \
  -RealEncryption=1 -D=2

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
