#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that an entry is removed from the bridging table when the NetKey is removed
#
# Test procedure:
# 1. Tester configures itself and creates subnets equal to number of non-bridge nodes.
# 2. Tester provisions and configures Subnet Bridge node.
# 3. Tester provisions and configures non-bridge nodes for each subnet.
# 4. Tester sends DATA and GET messages to the device 1 encrypted with primary key and verifies that
# device 1 sends a STATUS message with the content of the DATA message.
# 5. Tester removes the NetKey from Subnet Bridge node.
# 6. Tester sends a GET message and verifies that no response is received.
# 7. Tester retrieves entries from the bridging table and verifies that the entry with NetKeyIndex2
# set to the removed NetKey is removed.

RunTest mesh_brg_net_key_remove \
	brg_tester_net_key_remove brg_bridge_simple brg_device_simple brg_device_simple

overlay=overlay_psa_conf
RunTest mesh_brg_net_key_remove_psa \
	brg_tester_net_key_remove brg_bridge_simple brg_device_simple brg_device_simple
