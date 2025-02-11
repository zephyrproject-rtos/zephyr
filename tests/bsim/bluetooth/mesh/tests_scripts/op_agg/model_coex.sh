#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that Opcode aggregator server and client can coexist on the same device.
# In this test scenario, the DUT has a sequence in progress on the Opcode
# Aggregator client model that is interrupted by an incoming sequence to the Opcode
# Aggregator server model. The test verifies that both the incoming and outgoing
# sequence on the DUT completes successfully.
#
# Test procedure:
# 1. Both Tester and DUT device initialize both Opcode aggregator server and client.
#    Both devices starts an Opcode aggregator sequence and populates the buffer.
# 2. The Tester device immediately starts sending the sequence.
# 3. The DUT device waits, and verifies that the sequence is correctly received.
#    Then it starts sending its own aggregated sequence.
# 4. The Tester device confirms that it received all status messages related to its
#    own aggregated sequence from the DUT device, then it verifies that the
#    aggregated sequence from the DUT device is correctly received.
# 5. Finally, the DUT device waits and confirms that it received all status messages
#    related to its own aggregated sequence from the cli device.
RunTest mesh_op_agg_model_coex \
	op_agg_tester_model_coex op_agg_dut_model_coex

overlay=overlay_psa_conf
RunTest mesh_op_agg_model_coex \
	op_agg_tester_model_coex op_agg_dut_model_coex
