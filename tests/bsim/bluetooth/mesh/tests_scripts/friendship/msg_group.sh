#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test receives on group and virtual addresses in the LPN
RunTest mesh_friendship_msg_group \
	friendship_lpn_group \
	friendship_other_group \
	friendship_friend_group

overlay=overlay_psa_conf
RunTest mesh_friendship_msg_group_psa \
	friendship_lpn_group \
	friendship_other_group \
	friendship_friend_group
