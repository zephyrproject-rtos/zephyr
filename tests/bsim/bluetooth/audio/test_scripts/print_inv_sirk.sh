#!/usr/bin/env bash
#
# Copyright (c) 2023 Codecoup
#
# SPDX-License-Identifier: Apache-2.0

# CSIP test invalid behaviour print non-existing SIRK

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=20

cd ${BSIM_OUT_PATH}/bin

SIMULATION_ID="csip_invalid_sirk"

Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=csip_set_member_print_sirk_invalid \
  -RealEncryption=1 -rs=2 -argstest rank 1

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=4 -sim_length=60e6 $@

wait_for_background_jobs
