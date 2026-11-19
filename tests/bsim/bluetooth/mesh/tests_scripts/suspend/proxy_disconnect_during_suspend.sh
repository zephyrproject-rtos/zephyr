#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Proxy GATT client stays connected while the DUT suspends, then disconnects
# asynchronously. bt_mesh_resume() must succeed and proxy advertising must
# resume (Zephyr #125420 proxy leak).

overlay=overlay_gatt_conf
RunTest mesh_proxy_disconnect_during_suspend \
	suspend_dut_proxy_disconnect_during_suspend \
	suspend_tester_proxy_disconnect_during_suspend
