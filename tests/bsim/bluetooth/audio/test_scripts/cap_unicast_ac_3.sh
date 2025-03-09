#!/usr/bin/env bash
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

SIMULATION_ID="cap_unicast_ac_3"
VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=60

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd ${BSIM_OUT_PATH}/bin


function Execute_AC_3() {
    printf "\n\n======== Running CAP AC_3 with %s =========\n\n" $1

    Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=0 -testid=cap_initiator_ac_3 \
        -RealEncryption=1 -rs=23 -D=2 -argstest sink_preset $1 source_preset $2

    Execute ./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf \
        -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} -d=1 -testid=cap_acceptor_unicast \
        -RealEncryption=1 -rs=46 -D=2

    # Simulation time should be larger than the WAIT_TIME in common.h
    Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${SIMULATION_ID} \
        -D=2 -sim_length=60e6 ${@:3}

    wait_for_background_jobs
}

set -e # Exit on error

Execute_AC_3 8_1_1 8_1_1
Execute_AC_3 8_2_1 8_2_1
Execute_AC_3 16_1_1 16_1_1
Execute_AC_3 16_2_1 16_2_1
Execute_AC_3 24_1_1 24_1_1
Execute_AC_3 24_2_1 24_2_1
Execute_AC_3 32_1_1 32_1_1
Execute_AC_3 32_2_1 32_2_1
# No sent callback on peripheral and no RX on peripheral
# https://github.com/zephyrproject-rtos/zephyr/issues/83585
# Execute_AC_3 441_1_1 441_1_1
# ASSERTION FAIL [err == ((isoal_status_t) 0x00)] @
# zephyr/subsys/bluetooth/controller/hci/hci_driver.c:489
# https://github.com/zephyrproject-rtos/zephyr/issues/83586
# Execute_AC_3 441_2_1 441_2_1
# No sent callback on peripheral and no RX on peripheral
# https://github.com/zephyrproject-rtos/zephyr/issues/83585
# Execute_AC_3 48_1_1 48_1_1
Execute_AC_3 48_2_1 48_2_1
Execute_AC_3 48_3_1 48_3_1
Execute_AC_3 48_4_1 48_4_1
Execute_AC_3 48_5_1 48_5_1
Execute_AC_3 48_6_1 48_6_1
