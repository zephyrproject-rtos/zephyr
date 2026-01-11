#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test checks that vendor models are able to receive messages using
# SIG-defined opcodes, which is needed for vendor models to communicate
# with SIG models.
#
# Test scenario:
# Device 2 has a vendor model that defines a handler for a specific 1-byte (SIG)
# opcode.
# Device 1 sends a message using the 1-byte (SIG) opcode to device 2.
# The vendor model on device 2 receives the message.

RunTest mesh_access_vnd_rcv_sig_msg \
	access_tx_transmit_sig_msg_to_vnd access_rx_transmit_sig_msg_to_vnd
