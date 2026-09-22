#!/usr/bin/env bash
# Copyright 2025 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test scenario:
# Allocate an advertiser buffer using bt_mesh_adv_buf_create().
# Submit the advertiser buffer using bt_mesh_adv_send().
# call `bt_mesh_adv_terminate` to terminate the ongoing transmission.
# expect the advertiser terminated and cd->end not be called.
RunTest mesh_adv_tx_send_terminate adv_tx_send_terminate

overlay="overlay_multi_adv_sets_conf"
RunTest mesh_adv_tx_send_terminate_multi_adv_sets adv_tx_send_terminate
