#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test communication between the LPN and a third mesh device.
# Test scenario:
# 1. LPN and Friend establish friendship.
# 2. LPN sends unsegmented data to the third mesh device and receives unsegmented data back.
# 3. LPN sends segmented data to the third mesh device and receives segmented data back.
# 4. LPN publishes unsegmented data to the third device with friendship credentials.
#    Friend should relay data to the third device with network credentials.
#    The third mesh receives published data.
# 5. LPN terminates friendship  and gets an event about termination.
# 6. LPN sends unsegmented data to the third device outside of friendship and
#    receives unsegmented data back.
RunTest mesh_friendship_msg_mesh \
	friendship_lpn_msg_mesh \
	friendship_other_msg \
	friendship_friend_est

overlay=overlay_lpn_scan_on_conf
RunTest mesh_friendship_msg_mesh_lpn_scan_on \
	friendship_lpn_msg_mesh \
	friendship_other_msg \
	friendship_friend_est

overlay=overlay_psa_conf
RunTest mesh_friendship_msg_mesh_psa \
	friendship_lpn_msg_mesh \
	friendship_other_msg \
	friendship_friend_est
