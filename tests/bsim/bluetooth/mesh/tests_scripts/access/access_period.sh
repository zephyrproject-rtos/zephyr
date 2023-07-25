#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_access_per_pub \
	access_tx_period access_rx_period

conf=prj_mesh1d1_conf
RunTest mesh_access_per_pub_1d1 \
	access_tx_period access_rx_period
