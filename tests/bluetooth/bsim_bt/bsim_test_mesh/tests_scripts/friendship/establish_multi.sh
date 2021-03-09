#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Establish multiple different friendships concurrently.
# Note: The number of LPNs must match CONFIG_BT_MESH_FRIEND_LPN_COUNT.
RunTest mesh_friendship_establish_multi \
	friendship_friend_est_multi \
	friendship_lpn_est \
	friendship_lpn_est \
	friendship_lpn_est \
	friendship_lpn_est \
	friendship_lpn_est
