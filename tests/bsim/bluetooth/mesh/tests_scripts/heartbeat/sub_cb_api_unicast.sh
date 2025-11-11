#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Run unicast heartbeat subscription to a device
RunTest mesh_heartbeat_sub_cb_api_unicast \
	heartbeat_publish_unicast \
	heartbeat_publish_unicast \
	heartbeat_subscribe_unicast
