#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test friendship re-establishment
RunTest mesh_friendship_re_est \
	friendship_friend_est \
	friendship_lpn_re_est

overlay=overlay_psa_conf
RunTest mesh_friendship_re_est_psa \
	friendship_friend_est \
	friendship_lpn_re_est
