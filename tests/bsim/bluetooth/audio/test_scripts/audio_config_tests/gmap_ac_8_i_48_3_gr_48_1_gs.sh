#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "$0")/_ac_common.sh

ac_config=8_i ac_tx_preset=48_3_gr ac_rx_preset=48_1_gs ac_acc_cnt=1 Execute_gmap_unicast_ac $@
