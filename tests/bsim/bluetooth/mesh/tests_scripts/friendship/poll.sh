#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test poll timeout
RunTest mesh_friendship_poll \
	friendship_friend_est \
	friendship_lpn_poll

overlay=overlay_psa_conf
RunTest mesh_friendship_poll_psa \
	friendship_friend_est \
	friendship_lpn_poll
