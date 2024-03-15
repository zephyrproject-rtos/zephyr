#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test transmission to virtual addresses with collision
RunTest mesh_transport_va_collision transport_tx_va_collision transport_rx_va_collision

overlay=overlay_psa_conf
RunTest mesh_transport_va_collision_psa transport_tx_va_collision transport_rx_va_collision
