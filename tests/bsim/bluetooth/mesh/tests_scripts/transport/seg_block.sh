#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_transport_seg_block transport_tx_seg_block transport_rx_seg_block

conf=prj_mesh1d1_conf
RunTest mesh_transport_seg_block_1d1 transport_tx_seg_block transport_rx_seg_block
