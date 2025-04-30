#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "$0")/_ac_common.sh

# No sent callback on peripheral and no RX on peripheral
# https://github.com/zephyrproject-rtos/zephyr/issues/83585
# ac_config=7_i ac_tx_preset=441_1_1 ac_rx_preset=441_1_1 ac_acc_cnt=1 Execute_cap_unicast_ac $@
