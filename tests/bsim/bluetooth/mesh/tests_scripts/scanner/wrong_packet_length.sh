#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_scanner_wrong_packet_length \
	scanner_tx_wrong_packet_length \
	scanner_rx_invalid_packet

conf=prj_mesh1d1_conf
RunTest mesh_scanner_wrong_packet_length_1d1 \
	scanner_tx_wrong_packet_length \
	scanner_rx_invalid_packet
