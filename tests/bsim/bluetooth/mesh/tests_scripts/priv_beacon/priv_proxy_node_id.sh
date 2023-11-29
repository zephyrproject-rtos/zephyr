#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Private Node ID advertisment
#
# Test procedure:
# 0. TX device disables GATT proxy and enables the Private Node
#    Identity state for the network. Then it waits for the
#    advertisment to complete.
# 1. RX device enables scanner and scans for Private Node ID advertisements.
# 2. RX device scans for a single Private Node ID advertisement and stores
#    the random field of that message. Then it waits for the
#    advertisement to complete.
# 3. TC device verifies that the previous advertisement is finished
#    and enables the Private Node Identity state for the network
#    again.
# 4. RX device scans for a another Private Node ID advertisement and compares
#    the random field of that message with the random field
#    of the previous Net ID.
# 5. Test passes if the random field of the two Private Node ID advertisements
#    are NOT equal.

overlay=overlay_gatt_conf
RunTest mesh_priv_proxy_node_id \
	beacon_tx_priv_node_id \
	beacon_rx_priv_node_id

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_priv_proxy_node_id \
	beacon_tx_priv_node_id \
	beacon_rx_priv_node_id
