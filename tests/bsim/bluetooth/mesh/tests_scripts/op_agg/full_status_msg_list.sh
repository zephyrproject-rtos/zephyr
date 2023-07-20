#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the OPcode aggregator (op agg) SIG model is able to send and receive a 380-byte
# access payload message and that the status item messages are received in the correct order.
# Spec 3.7.2: "With a 32-bit TransMIC field, the maximum size of the Access message is 380 octets."
# Spec 4.4.19.2: "When an Access message or an empty item is added to the message results list,
# it shall be located at the same index as the corresponding Access message from
# the message request list."

# Test procedure:
# 1. Initialize and configure op agg client and server instances.
# 2. The client configures op agg context.
# 3. The client sends X vendor model messages, making up an op agg sequence message of 380 bytes.
# 4. The client sends the sequence message with the previously configured context,
#    expecting an op agg status message of 380 bytes in return.
# 5. The server keeps track of the number of received messages and pass when X messages have been
#    received.
# 6. The client keeps track of the number of received status messages. When X messages have been
#    received, the client pass if the sequence of received status messages corresponds to the order
#    in which the messages were sent, or the test fails.
conf=prj_mesh1d1_conf
RunTest mesh_op_agg_test_max_access_payload \
	op_agg_cli_max_len_sequence_msg_send op_agg_srv_max_len_status_msg_send

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_op_agg_test_max_access_payload_psa \
	op_agg_cli_max_len_sequence_msg_send op_agg_srv_max_len_status_msg_send
