#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_access_pub_period_delayable_retr \
	access_tx_period_delayable access_rx_period_delayable

overlay=overlay_psa_conf
RunTest mesh_access_pub_period_delayable_retr_psa \
	access_tx_period_delayable access_rx_period_delayable
