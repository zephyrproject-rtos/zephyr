#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test buffer management by filling all the buffer and sending them all in order.
RunTest mesh_adv_send_order adv_tx_send_order adv_rx_receive_order

conf=prj_mesh1d1_conf
RunTest mesh_adv_send_order_1d1 adv_tx_send_order adv_rx_receive_order

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_adv_send_order_psa adv_tx_send_order adv_rx_receive_order
