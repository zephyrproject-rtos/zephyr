#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test proxy connection
overlay=overlay_gatt_conf
RunTest mesh_adv_proxy_conn adv_tx_proxy_conn adv_rx_proxy_conn

overlay=overlay_gatt_separate_conf
RunTest mesh_adv_proxy_conn_separate adv_tx_proxy_conn adv_rx_proxy_conn

overlay="overlay_gatt_conf_overlay_multi_adv_sets_conf"
RunTest mesh_adv_proxy_conn_multi_adv_sets adv_tx_proxy_conn adv_rx_proxy_conn
