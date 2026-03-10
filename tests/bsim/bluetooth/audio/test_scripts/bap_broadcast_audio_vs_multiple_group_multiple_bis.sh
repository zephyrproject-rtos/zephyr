#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=120

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

SIMULATION_ID="bap_broadcast_audio_vs_multiple_group_multiple_bis"

# The test has been ignored for nrf54l15bsim until
# https://github.com/zephyrproject-rtos/zephyr/issues/98941 has been fixed, as it currently won't
# pass for nrf54l15bsim.
if [ "${BOARD_TS}" == "nrf54l15bsim_nrf54l15_cpuapp" ]; then
    echo "Skipping test for ${BOARD_TS}"
    exit 0
fi

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=broadcast_source \
  -RealEncryption=1 -rs=23 -D=2 -argstest subgroup_cnt 2 streams_per_subgroup_cnt 2 vs_codec

Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
  -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=broadcast_sink \
  -RealEncryption=1 -rs=27 -D=2

# Simulation time should be larger than the WAIT_TIME in common.h
Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
