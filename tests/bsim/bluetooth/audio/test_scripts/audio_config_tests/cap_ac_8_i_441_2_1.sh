#!/usr/bin/env bash
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "$0")/_ac_common.sh

# ASSERTION FAIL [err == ((isoal_status_t) 0x00)] @
# zephyr/subsys/bluetooth/controller/hci/hci_driver.c:489
# https://github.com/zephyrproject-rtos/zephyr/issues/83586
# ac_config=8_i ac_tx_preset=441_2_1 ac_rx_preset=441_2_1 ac_acc_cnt=1 Execute_cap_unicast_ac $@
