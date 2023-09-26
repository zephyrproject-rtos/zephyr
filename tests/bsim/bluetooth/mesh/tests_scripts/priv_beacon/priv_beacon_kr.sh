#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Private Beacon advertising during Key Refresh procedure
conf=prj_mesh1d1_conf
RunTest mesh_priv_beacon_on_key_refresh \
  beacon_rx_on_key_refresh \
  beacon_tx_priv_on_key_refresh \
  -- -argstest rand-int=1

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_priv_beacon_on_key_refresh_psa \
  beacon_rx_on_key_refresh \
  beacon_tx_priv_on_key_refresh \
  -- -argstest rand-int=1
