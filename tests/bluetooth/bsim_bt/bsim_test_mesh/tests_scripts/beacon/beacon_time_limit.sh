#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_beacon_time_limit \
        beacon_tx_beacon_secure \
        beacon_tx_beacon_secure \
        beacon_tx_beacon_secure \
        beacon_tx_beacon_secure \
        beacon_rx_beacon_secure
