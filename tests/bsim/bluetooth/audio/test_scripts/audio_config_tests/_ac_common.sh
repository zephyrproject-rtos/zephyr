#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

VERBOSITY_LEVEL=2
EXECUTE_TIMEOUT=120
BSIM_EXE=./bs_${BOARD_TS}_tests_bsim_bluetooth_audio_prj_conf

cd ${BSIM_OUT_PATH}/bin

function Execute_ac() {
    if [[ -n $ac_tx_preset ]]; then
        ac_tx_preset_arg="${ac_tx_preset_arg_name} ${ac_tx_preset}"
    else
        ac_tx_preset_arg=""
    fi

    if [[ -n $ac_rx_preset ]]; then
        ac_rx_preset_arg="source_preset ${ac_rx_preset}"
    else
        ac_rx_preset_arg=""
    fi


    sim_id="${ac_profile}_ac_${ac_config}_${ac_tx_preset}_${ac_rx_preset}"

    printf "\n\n======== Running %s AC_%s with %s %s =========\n\n" \
        "${ac_profile}" "${ac_config}" "${ac_tx_preset_arg}" "${ac_rx_preset_arg}"

    let ac_device_count=1+${ac_acc_cnt}
    ac_ini_name=${ac_ini_name}_${ac_config}

    Execute ${BSIM_EXE} -v=${VERBOSITY_LEVEL} -s=${sim_id} -d=0 -testid=${ac_ini_name} \
        -RealEncryption=1 -rs=23 -D=${ac_device_count} \
        -argstest ${ac_tx_preset_arg} ${ac_rx_preset_arg}

    for i in $(seq 1 $ac_acc_cnt); do
        let rs=${i}*7 # ensure unique random seed value per acceptor

        Execute ${BSIM_EXE} -v=${VERBOSITY_LEVEL} -s=${sim_id} -d=${i} -testid=${ac_acc_name} \
            -RealEncryption=1 -rs=${rs} -D=${ac_device_count}
    done

    Execute ./bs_2G4_phy_v1 -v=${VERBOSITY_LEVEL} -s=${sim_id} -D=${ac_device_count} \
        -sim_length=60e6 $@

    wait_for_background_jobs
}

function Execute_cap_unicast_ac() {
    ac_profile="CAP" ac_ini_name=cap_initiator_ac ac_acc_name=cap_acceptor_unicast \
        ac_tx_preset_arg_name=sink_preset Execute_ac $@
}

function Execute_cap_broadcast_ac() {
    ac_profile="CAP" ac_ini_name=cap_initiator_ac ac_acc_name=cap_acceptor_broadcast \
        ac_tx_preset_arg_name=preset Execute_ac $@
}

function Execute_gmap_unicast_ac() {
    ac_profile="GMAP" ac_ini_name=gmap_ugg_ac ac_acc_name=gmap_ugt \
        ac_tx_preset_arg_name=sink_preset Execute_ac $@
}

function Execute_gmap_broadcast_ac() {
    ac_profile="GMAP" ac_ini_name=gmap_ugg_ac ac_acc_name=cap_acceptor_broadcast \
        ac_tx_preset_arg_name=broadcast_preset Execute_ac $@
}
