#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test checks that the LPN can complete friendship establishment while a third node keeps
# sending unicast mesh traffic directly to the LPN so that those frames may land in the LPN receive
# windows opened for Friend Offer / polling (MshPRT v1.1.1 3.6.6.2 / 3.6.6.4.1: Friend response
# handling must not be starved by unrelated ADV traffic).
#
# 3 roles are used: Friend, LPN, and another mesh device (third node) that does not participate in
# the friendship.
#
# Topology (all nodes in mutual radio range):
#
#              Other (sources stress unicast to LPN)
#                    |
#    Friend ←→ LPN   |
#
# Test procedure (high level):
# 1. Friend enables the Friend feature and waits for friendship to be established.
# 2. LPN enables LPN and waits for BT_MESH_TEST_LPN_ESTABLISHED while the stack retries in a tight
#    loop, failing if establishment takes too long or if retry polls are observed during
#    establishment.
# 3. Third node, after a short delay, repeatedly sends unicast messages to the LPN address for the
#    duration of the run (overlapping LPN receive windows during establishment).
# 4. After establishment, LPN and Friend both verify the friendship is still present (no unexpected
#    termination).

RunTest mesh_friendship_rx_window_filter \
	friendship_lpn_rx_window_filter \
	friendship_other_rx_window_filter \
	friendship_friend_rx_window_guard
