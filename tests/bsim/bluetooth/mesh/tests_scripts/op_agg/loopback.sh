#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that an Opcode aggregated message can be sent and received on loopback.
#
# Test procedure:
# 1. The device initializes both Opcode aggregator server and client.
#    The device starts an Opcode aggregator sequence and populates the buffer.
# 2. The device starts sending the sequence on loopback.
# 3. The device verifies that the sequence is correctly received by the server model.
# 4. The device confirms that the client model received all status messages.
RunTest mesh_op_agg_model_coex_loopback \
	op_agg_dut_model_coex_loopback

overlay=overlay_psa_conf
RunTest mesh_op_agg_model_coex_loopback \
	op_agg_dut_model_coex_loopback
