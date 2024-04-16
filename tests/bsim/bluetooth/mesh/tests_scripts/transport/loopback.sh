#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest transport_loopback transport_tx_loopback transport_rx_none

conf=prj_mesh1d1_conf
RunTest transport_loopback_1d1 transport_tx_loopback transport_rx_none

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest transport_loopback_psa transport_tx_loopback transport_rx_none
