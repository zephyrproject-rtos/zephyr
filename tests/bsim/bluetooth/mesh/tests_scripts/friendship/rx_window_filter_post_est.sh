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
# 2. LPN enables LPN, waits for BT_MESH_TEST_LPN_ESTABLISHED, then repeatedly polls the Friend
#    with spacing for each Friend Update receive window to complete.
# 3. Third node, after a fixed delay, sends unicast toward the LPN at a moderate rate so some
#    frames overlap LPN receive windows without routinely colliding with Friend traffic.
# 4. LPN asserts no retry polls, no unexpected friendship termination, and (with CONFIG_BT_TESTING)
#    no ADV policy violations when CONFIG_BT_MESH_LPN_ADV_FILTER is enabled.

RunTest mesh_friendship_rx_window_filter_post_est \
	friendship_lpn_rx_window_filter_post_est \
	friendship_other_rx_window_filter_post_est \
	friendship_friend_rx_window_guard
