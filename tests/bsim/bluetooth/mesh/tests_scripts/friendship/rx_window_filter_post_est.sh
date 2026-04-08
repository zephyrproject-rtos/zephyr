#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test checks that after friendship is already established, the LPN still ignores direct mesh
# traffic from a third node when that traffic arrives during LPN receive windows opened for Friend
# polling. The LPN must poll the Friend without falling into retry polls, and the friendship must
# remain stable.
#
# 3 roles are used: Friend, LPN, and another mesh device (third node) that only sends unicast
# stress traffic toward the LPN.
#
# Topology (all nodes in mutual radio range):
#
#              Other (sources stress unicast to LPN)
#                    |
#    Friend ←→ LPN   |
#
# Test procedure (high level):
# 1. Friend enables the Friend feature and waits for friendship establishment from its side.
# 2. LPN enables LPN, waits for BT_MESH_TEST_LPN_ESTABLISHED, then performs multiple manual polls
#    separated by sleeps so receive windows occur while the third node is active.
# 3. Third node waits until after friendship is expected to be up, then runs a longer loop of
#    unicast sends toward the LPN (overlapping LPN receive windows after establishment).
# 4. LPN asserts no retry polls were counted during the poll loop; Friend and LPN assert the
#    friendship did not terminate unexpectedly.

RunTest mesh_friendship_rx_window_filter_post_est \
	friendship_lpn_rx_window_filter_post_est \
	friendship_other_rx_window_filter_post_est \
	friendship_friend_rx_window_guard
