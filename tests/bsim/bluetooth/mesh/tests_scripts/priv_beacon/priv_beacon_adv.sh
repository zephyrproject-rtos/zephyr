#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Private Beacon advertising on node supporting relay feature.
# Test Random value changes for different Random intervals (10s, 0 - on every beacon, 30s).
RunTest mesh_priv_beacon_adv beacon_rx_priv_adv beacon_tx_priv_adv

overlay=overlay_psa_conf
RunTest mesh_priv_beacon_adv_psa beacon_rx_priv_adv beacon_tx_priv_adv
