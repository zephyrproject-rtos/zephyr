#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="cap_broadcast_ac_14"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=60

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

function Execute_AC_14() {
    printf "\n\n======== Running CAP AC_14 with %s =========\n\n" $1

    Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=cap_initiator_ac_14 \
        -RealEncryption=1 -rs=23 -D=2 -argstest preset $1

    Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=cap_acceptor_broadcast \
        -RealEncryption=1 -rs=46 -D=2

    # Simulation time should be larger than the WAIT_TIME in common.h
    Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
        -D=2 -sim_length=60e6 ${@:2}

    wait_for_background_jobs
}

set -e # Exit on error

# Low latency tests
Execute_AC_14 8_1_1
Execute_AC_14 8_2_1
Execute_AC_14 16_1_1
Execute_AC_14 16_2_1
Execute_AC_14 24_1_1
Execute_AC_14 24_2_1
Execute_AC_14 32_1_1
Execute_AC_14 32_2_1
Execute_AC_14 441_1_1
Execute_AC_14 441_2_1
Execute_AC_14 48_1_1
Execute_AC_14 48_2_1
Execute_AC_14 48_3_1
Execute_AC_14 48_4_1
Execute_AC_14 48_5_1
Execute_AC_14 48_6_1

# High reliability
# Execute_AC_14 8_1_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 8_2_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 16_1_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 16_2_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 24_1_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 24_2_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 32_1_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 32_2_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 441_1_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 441_2_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 48_1_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 48_2_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 48_3_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 48_4_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 48_5_2 # BT_ISO_FLAGS_ERROR
# Execute_AC_14 48_6_2 # BT_ISO_FLAGS_ERROR
