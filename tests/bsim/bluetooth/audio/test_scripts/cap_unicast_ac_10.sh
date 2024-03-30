#!/usr/bin/env bash
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="cap_unicast_ac_10"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=60

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

function Execute_AC_10() {
    printf "\n\n======== Running CAP AC_10 with %s =========\n\n" $1

    Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=cap_initiator_ac_10 \
        -RealEncryption=1 -rs=23 -D=2 -argstest source_preset $1

    Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=cap_acceptor_unicast \
        -RealEncryption=1 -rs=46 -D=2

    # Simulation time should be larger than the WAIT_TIME in common.h
    Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
        -D=2 -sim_length=60e6 ${@:2}

    wait_for_background_jobs
}

set -e # Exit on error

Execute_AC_10 8_1_1
Execute_AC_10 8_2_1
Execute_AC_10 16_1_1
Execute_AC_10 16_2_1
Execute_AC_10 24_1_1
Execute_AC_10 24_2_1
Execute_AC_10 32_1_1
Execute_AC_10 32_2_1
Execute_AC_10 441_1_1
Execute_AC_10 441_2_1
Execute_AC_10 48_1_1
Execute_AC_10 48_2_1
Execute_AC_10 48_3_1
Execute_AC_10 48_4_1
Execute_AC_10 48_5_1
Execute_AC_10 48_6_1
