#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that the subnet bridge can bridge traffic when either the
# incoming subnet, the outgoing subnet or both subnets are undergoing the
# Key Refresh Procedure.
#
# 3 roles are used in this test: Tester (Tester), Subnet Bridge node, and Mesh node.
#
# Subnets topology*:
#    Tester
#       |
#   (subnet 0)
#       |
#   Subnet Bridge (bridges subnets 0 <-> 1)
#       |
#   (subnet 1)
#       |
#     Node
#
# (*) - All nodes are in the tester's range
#
# Test procedure:
#  1. Tester configures itself and creates a subnet for the remote node.
#  2. Tester provisions and configures Subnet Bridge node.
#  3. Tester provisions and configures the non-bridge node for subnet 1.
#  4. For each network key:
#      a. Tester starts KRP on relevant nodes for the network key.
#      b. Tester sends DATA and GET messages to the non-bridge node encrypted
#         with the primary key and verifies that the non-bridge node sends a
#         STATUS message with the content of the DATA messages.
#      c. Tester triggers transition to KRP phase 0x02 for relevant nodes for
#         the network key.
#      d. Messaging is verified like in step 4b.
#      e. Tester triggers transition to KRP phase 0x03 for relevant nodes for
#         the network key.
#      f  Messaging is verified like in step 4b.
#  5. Tester starts KRP on all nodes for all network keys
#  6. Messaging is verified like in step 4b.
#  7. Tester triggers transition to KRP phase 0x02 for all nodes and net keys.
#  8. Messaging is verified like in step 4b.
#  9. Tester triggers transition to KRP phase 0x03 for all nodes and net keys.
# 10. Messaging is verified like in step 4b.

RunTest mesh_brg_net_key_refresh \
	brg_tester_key_refresh brg_bridge_simple brg_device_simple

overlay=overlay_psa_conf
RunTest mesh_brg_net_key_refresh_psa \
	brg_tester_key_refresh brg_bridge_simple brg_device_simple
