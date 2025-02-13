#!/usr/bin/env bash
# Copyright 2025 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test scenario:
# Allocate an advertiser buffer 1 using bt_mesh_adv_buf_create() with tag BT_MESH_ADV_TAG_LOCAL.
# Allocate an advertiser buffer 2 using bt_mesh_adv_buf_create() with tag BT_MESH_ADV_TAG_RELAY.
# Allocate an advertiser buffer 3 using bt_mesh_adv_buf_create() with tag BT_MESH_ADV_TAG_RELAY.
# Submit the advertiser buffer 1,2 and 3 using bt_mesh_adv_send().
# For not enable multiple advertising sets, expect the adv 1,2,3 are be sent by fifo order.
# For enable multiple advertising sets, expect the adv 1,2,3 are be sent by same time.

RunTest mesh_adv_tx_send_relay adv_tx_send_relay

overlay="overlay_multi_adv_sets_conf"
RunTest mesh_adv_tx_send_relay_multi_adv_sets adv_tx_send_relay
