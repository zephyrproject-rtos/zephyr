#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

conf=prj_low_lat_conf
RunTest mesh_transport_loopback_group transport_tx_loopback_group transport_rx_group
