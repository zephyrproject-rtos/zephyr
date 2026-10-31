#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that the BLOB Transfer Client:
#   - retransmits BLOB Information Get the configured number of times and
#     reports every target as lost when no BLOB Transfer Server responds to the
#     Retrieve Capabilities procedure;
#   - ignores a BLOB Block Status received before it has started any procedure.
#
# A BLOB Block Status is only expected in the Send Block and Determine Block
# Status sub-procedures (MshMBTv1.0 Table 6.1). A client that has not started
# any procedure since boot has no transfer inputs and no target list, so it
# must drop the message without looking the sender up.
#
# Node layout:
#   Device 0: BLOB Transfer Client (0x0001), application key 0 bound.
#   Device 1: Mock BLOB Transfer Server (0x0002). A model with the BLOB
#             Transfer Server ID that accepts BLOB Information Get and never
#             answers. Application key 0 bound, subscribed to 0xc000.
#   Device 2: Same as device 1 (0x0003).
#
# Test procedure:
# 1. All devices provision and configure themselves.
# 2. At 1 s, each mock server sends a BLOB Block Status to the client on
#    application key 0: Status Success, Format No Missing Chunks, Block
#    Number 0, Chunk Size 16.
# 3. At 2 s, the client asserts that it is still in the idle state and starts
#    the Retrieve Capabilities procedure towards both servers through group
#    0xc000.
# 4. Each mock server counts the BLOB Information Get messages it receives.
# 5. Test asserts:
#      - Client survives the BLOB Block Status from step 2.
#      - Each mock server receives CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES
#        BLOB Information Get messages.
#      - Client reports both servers as lost, then calls the caps callback
#        without capabilities.
#      - Both targets are marked timed out and not acknowledged.

# The test instance sequence must stay as it is due to addressing scheme
RunTest blob_caps_no_rsp \
	blob_cli_caps_no_rsp blob_srv_caps_no_rsp blob_srv_caps_no_rsp
