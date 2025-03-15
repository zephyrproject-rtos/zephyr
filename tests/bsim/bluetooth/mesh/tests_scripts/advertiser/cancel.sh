#!/usr/bin/env bash
# Copyright 2025 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test scenario:
# Allocate an advertiser buffer 1 using bt_mesh_adv_buf_create().
# Allocate an advertiser buffer 2 using bt_mesh_adv_buf_create().
# Submit the advertiser buffer 1 and 2 using bt_mesh_adv_send().
# In the first buffer cb->send_start cancel buffer 2.
# Expect the second buffer cb->send_start not be called.

RunTest mesh_adv_tx_send_cancel adv_tx_send_cancel

overlay="overlay_multi_adv_sets_conf"
RunTest mesh_adv_tx_send_cancel_multi_adv_sets adv_tx_send_cancel
