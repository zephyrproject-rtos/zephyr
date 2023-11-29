#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Check that the LPN terminate callback does not trigger when there is no established
# connection.
#
# This covers a corner case scenario where the LPN has received a friend offer, but has
# not yet established a connection. In this case the LPN terminate callback should not
# be triggered if the LPN is disabled, which is monitored by this test.
RunTest mesh_lpn_terminate_cb_check \
	friendship_friend_est \
	friendship_lpn_term_cb_check

overlay=overlay_psa_conf
RunTest mesh_lpn_terminate_cb_check_psa \
	friendship_friend_est \
	friendship_lpn_term_cb_check
