#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Establish multiple different friendships concurrently. Perform BLOB transfer with BLOB Client
# on friend node and BLOB Server on LPNs.
# Note: The number of LPNs must match CONFIG_BT_MESH_FRIEND_LPN_COUNT.
conf=prj_mesh1d1_conf
RunTest blob_transfer_lpn \
	blob_cli_friend_pull \
	blob_srv_lpn_pull \
	blob_srv_lpn_pull \
	blob_srv_lpn_pull \
	blob_srv_lpn_pull \
	blob_srv_lpn_pull
