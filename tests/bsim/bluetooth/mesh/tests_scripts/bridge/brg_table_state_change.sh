#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that bridging table works correctly
#
# Test procedure:
# 1. Tester configures itself and creates subnets equal to number of non-bridge nodes.
# 2. Tester provisions and configures Subnet Bridge node.
# 3. Tester provisions and configures non-bridge nodes for each subnet.
# 4. While bridging table is empty, Tester sends a GET message to each node encrypted with primary
# key and verifies that no response is received.
# 5. Tester adds an entry to the bridging table to bridge traffic in one direction from tester to
# device 1
# 6. Tester sends a DATA and GET messages to device 1 encrypted with primary key and verifies that
# no response is received.
# 9. Tester adds a reverse entry to the bridging table to bridge traffic in the other direction from
# device 1 to tester.
# 10. Tester sends a GET message to device 1 encrypted with primary key and verifies that a STATUS
# message is received with the content of the previously sent DATA message.
# 11. Tester removes the reverse entry from the bridging table and updates direction of the first
# entry to bridge traffic in the both directions between tester and device 1.
# 12. Tester sends a GET message to device 1 encrypted with primary key and verifies that a STATUS
# message is received with the empty content.
RunTest mesh_brg_table_state_change \
	brg_tester_table_state_change brg_bridge_simple brg_device_simple brg_device_simple

overlay=overlay_psa_conf
RunTest mesh_brg_table_state_change_psa \
	brg_tester_table_state_change brg_bridge_simple brg_device_simple brg_device_simple
