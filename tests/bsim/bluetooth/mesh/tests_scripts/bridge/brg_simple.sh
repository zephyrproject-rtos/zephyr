#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test verifies that Subnet Bridge stops bridging messages when the Subnet Bridge state is
# disabled.
#
# 3 roles are used in this test: Tester (Tester), Subnet Bridge node, and Mesh node.
#
# Subnets topology*:
#           Tester
#             |
#         (subnet 0)
#            |
#       Subnet Bridge (bridges subnets 0 <-> 1, 0 <-> 2)
#        /         \
#    (subnet 1)  (subnet 2)
#       |          |
#     Node       Node
#
# (*) - All nodes are in the tester's range
#
# Test procedure:
# 1. Tester configures itself and creates subnets equal to number of non-bridge nodes.
# 2. Tester provisions and configures Subnet Bridge node to bridge the subnets.
# 3. Tester provisions and configures non-bridge nodes for each subnet.
# 4. Tester sends a DATA message to each node encrypted with primary key.
# 5. Nodes store the received messages.
# 6. Tester sends a GET message to each node encrypted with primary key.
# 7. Nodes send the stored messages back to the tester through a STATUS message encrypted
# with the key of the subnets they are provisioned to.
# 8. Tester verifies that each node received messages.
# 9. Tester disables Subnet Bridge state.
# 10. Tester sends a DATA message to each node encrypted with primary key.
# 11. Tester sends a GET message to each node encrypted with primary key.
# 12. Tester verifies that each node didn't receive DATA messages.

RunTest mesh_brg_simple \
	brg_tester_simple brg_bridge_simple brg_device_simple brg_device_simple

overlay=overlay_psa_conf
RunTest mesh_brg_simple_psa \
	brg_tester_simple brg_bridge_simple brg_device_simple brg_device_simple
