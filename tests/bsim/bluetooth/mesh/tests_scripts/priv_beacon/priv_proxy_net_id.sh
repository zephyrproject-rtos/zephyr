#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Private Net ID advertisment
#
# Test procedure:
# 0. TX device disables GATT proxy and enables Private GATT proxy.
# 1. RX device enables scanner and scans for Net ID advertisments.
# 2. RX device scans for a single Net ID advertisment and stores
#    the random field of that message.
# 3. RX device waits for 10 minutes.
# 4. RX device scans for a another Private Net ID advertisement and compares
#    the random field of that message with the random field
#    of the previous Private Net ID.
# 5. Test passes if the random field of the two Private Net ID advertisements
#    are NOT equal.

conf=prj_mesh1d1_conf
overlay=overlay_gatt_conf
RunTest mesh_priv_proxy_net_id \
	beacon_tx_priv_net_id \
	beacon_rx_priv_net_id

conf=prj_mesh1d1_conf
overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_priv_proxy_net_id \
	beacon_tx_priv_net_id \
	beacon_rx_priv_net_id
