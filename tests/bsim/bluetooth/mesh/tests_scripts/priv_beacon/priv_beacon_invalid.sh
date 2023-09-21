#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test if Private Beacons with invalid data do not affect device
conf=prj_mesh1d1_conf
RunTest mesh_priv_beacon_invalid beacon_rx_priv_invalid beacon_tx_priv_invalid

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_priv_beacon_invalid_psa beacon_rx_priv_invalid beacon_tx_priv_invalid
