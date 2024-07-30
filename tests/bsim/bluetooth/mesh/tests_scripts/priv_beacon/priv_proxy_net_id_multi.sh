#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Private Net ID advertisment with multiple networks
#
# Test procedure:
# 0. TX device disables GATT proxy, adds a second network to the device
#    and enables Private GATT proxy.
# 1. RX device enables scanner and scans for Private Net ID advertisements from
#    both of the networks.
# 2. Test passes when RX device verifies reception of one Private Net ID
#    advertisemen from each of the networks within the given time
#    limit.

overlay=overlay_gatt_conf
RunTest mesh_priv_proxy_net_id_multi \
	beacon_tx_priv_multi_net_id \
	beacon_rx_priv_multi_net_id

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_priv_proxy_net_id_multi \
	beacon_tx_priv_multi_net_id \
	beacon_rx_priv_multi_net_id
