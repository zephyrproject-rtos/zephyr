#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test transmission to virtual addresses with collision
RunTest mesh_transport_va_collision transport_tx_va_collision transport_rx_va_collision

conf=prj_mesh1d1_conf
RunTest mesh_transport_va_collision_1d1 transport_tx_va_collision transport_rx_va_collision

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_transport_va_collision_psa transport_tx_va_collision transport_rx_va_collision
