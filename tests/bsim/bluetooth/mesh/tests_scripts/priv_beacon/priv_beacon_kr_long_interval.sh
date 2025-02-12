#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Private Beacon advertising during Key Refresh procedure, with long Random Interval set.
# Random value is expected to change before Random Interval is reached due to
# Flags field change.
RunTest mesh_priv_beacon_on_key_refresh_long_interval \
  beacon_rx_on_key_refresh \
  beacon_tx_priv_on_key_refresh \
  -- -argstest rand-int=3

overlay=overlay_psa_conf
RunTest mesh_priv_beacon_on_key_refresh_long_interval_psa \
  beacon_rx_on_key_refresh \
  beacon_tx_priv_on_key_refresh \
  -- -argstest rand-int=3
