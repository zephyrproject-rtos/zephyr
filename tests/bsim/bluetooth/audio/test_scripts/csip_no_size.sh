#!/usr/bin/env bash
#
# Copyright (c) 2023-2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# CSIP test where there is no set size characteristic on the Set Members

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=180

cd ${BSIM_OUT_PATH}/bin

SIMULATION_ID="csip_no_size"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator \
  -RealEncryption=1 -rs=10 -D=4 -argstest no-size

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member \
  -RealEncryption=1 -rs=20 -D=4 -argstest rank 1 size 0

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member \
  -RealEncryption=1 -rs=30 -D=4 -argstest rank 2 size 0

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member \
  -RealEncryption=1 -rs=40 -D=4 -argstest rank 3 size 0

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=4 -sim_length=60e6 $@

wait_for_background_jobs
