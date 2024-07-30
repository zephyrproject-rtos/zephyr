#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Run all nodes heartbeat subscription to a device
RunTest mesh_heartbeat_sub_cb_api_all \
	heartbeat_publish_all \
	heartbeat_publish_all \
	heartbeat_subscribe_all

overlay=overlay_psa_conf
RunTest mesh_heartbeat_sub_cb_api_all_psa \
	heartbeat_publish_all \
	heartbeat_publish_all \
	heartbeat_subscribe_all
