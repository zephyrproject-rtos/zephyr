#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_access_publication_cancel \
	access_tx_cancel access_rx_cancel

overlay=overlay_psa_conf
RunTest mesh_access_publication_cancel_psa \
	access_tx_cancel access_rx_cancel
