#!/usr/bin/env bash
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="cap_unicast_ac_9_ii"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=60

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

function Execute_AC_9_II() {
    printf "\n\n======== Running CAP AC_9_II with %s =========\n\n" $1

    Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=cap_initiator_ac_9_ii \
        -RealEncryption=1 -rs=23 -argstest source_preset $1

    Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=cap_acceptor_unicast \
        -RealEncryption=1 -rs=46

    Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=2 -testid=cap_acceptor_unicast \
        -RealEncryption=1 -rs=69

    # Simulation time should be larger than the WAIT_TIME in common.h
    Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
        -D=3 -sim_length=60e6 ${@:2}

    wait_for_background_jobs
}

set -e # Exit on error

Execute_AC_9_II 8_1_1
Execute_AC_9_II 8_2_1
Execute_AC_9_II 16_1_1
Execute_AC_9_II 16_2_1
Execute_AC_9_II 24_1_1
Execute_AC_9_II 24_2_1
Execute_AC_9_II 32_1_1
Execute_AC_9_II 32_2_1
# Execute_AC_9_II 441_1_1 # ASSERTION FAIL [iso_interval_us >= cig->c_sdu_interval]
# Execute_AC_9_II 441_2_1 # ASSERTION FAIL [iso_interval_us >= cig->c_sdu_interval]
Execute_AC_9_II 48_1_1
Execute_AC_9_II 48_2_1
Execute_AC_9_II 48_3_1
Execute_AC_9_II 48_4_1
Execute_AC_9_II 48_5_1
Execute_AC_9_II 48_6_1
