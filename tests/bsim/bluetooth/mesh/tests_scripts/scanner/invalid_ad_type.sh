#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_scanner_invalid_ad_type \
	scanner_tx_invalid_ad_type \
	scanner_rx_invalid_packet
