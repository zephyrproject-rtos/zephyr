#!/usr/bin/env bash
#
# Copyright (c) 2023 Codecoup
#
# SPDX-License-Identifier: Apache-2.0

# CSIP test where one of set members has different sirk than others

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=20

cd ${BSIM_OUT_PATH}/bin

SIMULATION_ID="csip_invalid_sirk"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=csip_set_coordinator \
  -RealEncryption=1 -rs=1 -argstest inv_sirk

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member_sirk \
  -RealEncryption=1 -rs=2 -argstest rank 1 sirk "cdcc72dd868ccdce22fda121097d7d46"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=csip_set_member_sirk \
  -RealEncryption=1 -rs=3 -argstest rank 2 sirk "cdcc72dd868ccdce22fda121097d7d45"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=3 -testid=csip_set_member_sirk \
  -RealEncryption=1 -rs=4 -argstest rank 3 sirk "cdcc72dd868ccdce22fda121097d7d45"

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=4 -sim_length=60e6 $@

wait_for_background_jobs
