#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test interleaving of Secure Network Beacons and Private Beacons advertisements.
# Setup: RX device as BT observer, TX device with enabled SNB and disabled PRB
# Procedure:
# - First SNB is advertised after Beacon Interval
# - PRB is enabled, SNB disabled, before next interval
# - First PRB is advertised
# - IVU is initiated
# - Second PRB is advertised with IVU flag
# - PRB is disabled, SNB enabled: second SNB is advertised
# - KR is initiated, third SNB is advertised with new flags (IVU + KR)
# - PRB is enabled, SNB is disabled. Third PRB is advertised
RunTest mesh_priv_beacon_interleave beacon_rx_priv_interleave beacon_tx_priv_interleave

overlay=overlay_psa_conf
RunTest mesh_priv_beacon_interleave_psa beacon_rx_priv_interleave beacon_tx_priv_interleave
