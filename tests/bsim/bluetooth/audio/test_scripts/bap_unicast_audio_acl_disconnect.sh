#!/usr/bin/env bash
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="unicast_audio_acl_disconnect"
VERBOSITY_LEVEL=2

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== Unicast Audio ACL Disconnect test =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=unicast_client_acl_disconnect \
  -RealEncryption=1 -rs=23 -D=2

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=unicast_server_acl_disconnect \
  -RealEncryption=1 -rs=28 -D=2

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
