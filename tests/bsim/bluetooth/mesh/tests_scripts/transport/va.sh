#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_transport_va transport_tx_va transport_rx_va

conf=prj_mesh1d1_conf
RunTest mesh_transport_va_1d1 transport_tx_va transport_rx_va
