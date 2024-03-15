#!/usr/bin/env bash
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="gmap_unicast_ac_11_i"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=60

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin

function Execute_AC_11_I() {
    printf "\n\n======== Running GMAP AC_11_I with %s and %s =========\n\n" $1 $2

    Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=gmap_ugg_ac_11_i -RealEncryption=1 \
        -rs=23 -D=2 -argstest sink_preset $1 source_preset $2

    Execute ./bs_${BOARD}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=gmap_ugt -RealEncryption=1 \
        -rs=46 -D=2

    # Simulation time should be larger than the WAIT_TIME in common.h
    Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
        -D=2 -sim_length=60e6 ${@:3}

    wait_for_background_jobs
}

set -e # Exit on error

Execute_AC_11_I 32_1_gr 16_1_gs
Execute_AC_11_I 32_2_gr 16_2_gs
Execute_AC_11_I 48_1_gr 16_1_gs
Execute_AC_11_I 48_2_gr 16_2_gs
Execute_AC_11_I 32_1_gr 32_1_gs
Execute_AC_11_I 32_2_gr 32_2_gs
Execute_AC_11_I 48_1_gr 32_1_gs
Execute_AC_11_I 48_2_gr 32_2_gs
Execute_AC_11_I 48_1_gr 48_1_gs
Execute_AC_11_I 48_2_gr 48_2_gs
Execute_AC_11_I 48_3_gr 32_1_gs
Execute_AC_11_I 48_4_gr 32_2_gs
Execute_AC_11_I 48_3_gr 48_1_gs
Execute_AC_11_I 48_4_gr 48_2_gs
