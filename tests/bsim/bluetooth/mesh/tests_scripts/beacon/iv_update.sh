#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_beacon_on_iv_update \
	beacon_tx_on_iv_update \
	beacon_rx_on_iv_update

conf=prj_mesh1d1_conf
RunTest mesh_beacon_on_iv_update_1d1 \
	beacon_tx_on_iv_update \
	beacon_rx_on_iv_update

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_beacon_on_iv_update_psa \
	beacon_tx_on_iv_update \
	beacon_rx_on_iv_update
