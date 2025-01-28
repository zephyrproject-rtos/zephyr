#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

overlay=overlay_pst_conf
RunTest mesh_replay_attack \
    rpc_tx_immediate_replay_attack \
    rpc_rx_immediate_replay_attack -flash=../results/mesh_replay_attack/flash.bin -flash_erase

overlay=overlay_pst_conf
RunTest mesh_replay_attack \
    rpc_tx_power_replay_attack \
    rpc_rx_power_replay_attack -flash=../results/mesh_replay_attack/flash.bin -flash_rm

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTest mesh_replay_attack_psa \
    rpc_tx_immediate_replay_attack \
    rpc_rx_immediate_replay_attack -flash=../results/mesh_replay_attack_psa/flash.bin -flash_erase

overlay="overlay_pst_conf_overlay_ss_conf_overlay_psa_conf"
RunTest mesh_replay_attack_psa \
    rpc_tx_power_replay_attack \
    rpc_rx_power_replay_attack -flash=../results/mesh_replay_attack_psa/flash.bin -flash_rm
