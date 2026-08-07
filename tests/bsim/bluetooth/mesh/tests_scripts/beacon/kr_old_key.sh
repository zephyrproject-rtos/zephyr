#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_beacon_kr_old_key \
	beacon_tx_kr_old_key \
	beacon_rx_kr_old_key
