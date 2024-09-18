#!/usr/bin/env bash
#
# Copyright (c) 2021-2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="bap_unicast_client_reconf_group"
VERBOSITY_LEVEL=2

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

printf "\n\n======== BAP Unicast Client Reconfigure Group parameters test =========\n\n"

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=unicast_client_reconf_group -rs=23 -D=1

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=1 -sim_length=60e6 $@

wait_for_background_jobs
