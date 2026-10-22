#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_beacon_on_iv_update \
	beacon_tx_on_iv_update \
	beacon_rx_on_iv_update
