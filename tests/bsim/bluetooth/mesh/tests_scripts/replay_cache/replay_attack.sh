#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

overlay=overlay_pst_conf
RunTest mesh_replay_attack \
	rpc_tx_immediate_replay_attack \
	rpc_rx_immediate_replay_attack

overlay=overlay_pst_conf
RunTest mesh_replay_attack \
	rpc_tx_power_replay_attack \
	rpc_rx_power_replay_attack

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest mesh_replay_attack_1d1 \
	rpc_tx_immediate_replay_attack \
	rpc_rx_immediate_replay_attack

conf=prj_mesh1d1_conf
overlay=overlay_pst_conf
RunTest mesh_replay_attack_1d1 \
	rpc_tx_power_replay_attack \
	rpc_rx_power_replay_attack
