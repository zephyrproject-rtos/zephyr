#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test LPN sending packets to a group and virtual address it subscribes to
RunTest mesh_friendship_lpn_loopback \
	friendship_lpn_loopback \
	friendship_friend_est

conf=prj_mesh1d1_conf
RunTest mesh_friendship_lpn_loopback_1d1 \
	friendship_lpn_loopback \
	friendship_friend_est

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_friendship_lpn_loopback_psa \
	friendship_lpn_loopback \
	friendship_friend_est
