#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "$0")/_ac_common.sh

# bap_stream_rx.c:66): ISO receive lost
# https://github.com/zephyrproject-rtos/zephyr/issues/84303
# ac_config=5 ac_tx_preset=441_2_1 ac_rx_preset=441_2_1 ac_acc_cnt=1 Execute_cap_unicast_ac $@
