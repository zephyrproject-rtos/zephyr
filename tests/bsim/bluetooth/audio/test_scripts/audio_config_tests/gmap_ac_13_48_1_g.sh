#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "$0")/_ac_common.sh

ac_config=13 ac_tx_preset=48_1_g ac_acc_cnt=1 Execute_gmap_broadcast_ac $@
