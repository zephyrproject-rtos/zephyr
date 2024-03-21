#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Send messages from the friend to the LPN
RunTest mesh_friendship_msg_frnd \
	friendship_friend_msg \
	friendship_lpn_msg_frnd

conf=prj_mesh1d1_conf
RunTest mesh_friendship_msg_frnd_1d1 \
	friendship_friend_msg \
	friendship_lpn_msg_frnd

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_friendship_msg_frnd_psa \
	friendship_friend_msg \
	friendship_lpn_msg_frnd
