#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_beacon_multiple_netkeys \
	beacon_tx_multiple_netkeys \
	beacon_rx_multiple_netkeys

conf=prj_mesh1d1_conf
RunTest mesh_beacon_multiple_netkeys_1d1 \
	beacon_tx_multiple_netkeys \
	beacon_rx_multiple_netkeys
