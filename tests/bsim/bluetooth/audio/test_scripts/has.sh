#!/usr/bin/env bash
#
# Copyright (c) 2022 Codecoup
#
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

SIMULATION_ID="has"
VERBOSITY_LEVEL=2

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Running HAS main (API) test =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=has \
  -RealEncryption=1 -rs=24 -D=2

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=has_client \
  -RealEncryption=1 -rs=46 -D=2

Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
