#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "$0")/_ac_common.sh

ac_config=1 ac_tx_preset=32_1_gr ac_acc_cnt=1 Execute_gmap_unicast_ac $@
