#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test buffer management by filling all the buffer and sending them in reversed order.
RunTest mesh_adv_reverse_order adv_tx_reverse_order adv_rx_receive_order

overlay=overlay_workq_sys_conf
RunTest mesh_adv_reverse_order_workq adv_tx_reverse_order adv_rx_receive_order

overlay="overlay_multi_adv_sets_conf"
RunTest mesh_adv_reverse_order_multi_adv_sets adv_tx_reverse_order adv_rx_receive_order

overlay=overlay_psa_conf
RunTest mesh_adv_reverse_order_psa adv_tx_reverse_order adv_rx_receive_order
