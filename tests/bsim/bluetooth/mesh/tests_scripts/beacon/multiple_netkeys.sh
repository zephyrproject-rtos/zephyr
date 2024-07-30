#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_beacon_multiple_netkeys \
	beacon_tx_multiple_netkeys \
	beacon_rx_multiple_netkeys

overlay=overlay_psa_conf
RunTest mesh_beacon_multiple_netkeys_psa \
	beacon_tx_multiple_netkeys \
	beacon_rx_multiple_netkeys
