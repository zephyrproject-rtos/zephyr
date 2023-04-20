#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test communication between the LPN and a third mesh device
overlay=overlay_low_lat_conf
RunTest mesh_friendship_msg_mesh_low_lat \
	friendship_lpn_msg_mesh \
	friendship_other_msg \
	friendship_friend_est

conf=prj_mesh1d1_conf
overlay=overlay_low_lat_conf
RunTest mesh_friendship_msg_mesh_low_lat_1d1 \
	friendship_lpn_msg_mesh \
	friendship_other_msg \
	friendship_friend_est
