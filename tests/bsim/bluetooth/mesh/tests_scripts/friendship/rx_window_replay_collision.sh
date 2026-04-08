#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test reproduces a deterministic replay / collision window: the third node sends a single
# group message but configures aggressive network-layer retransmissions so identical ADV frames
# spread over time while the LPN has opened a receive window to poll the Friend. The LPN must still
# receive that group message and must not miss the Friend control path in a way that triggers
# retry polls (which would indicate the Friend response was effectively starved or mishandled).
#
# 3 roles are used: Friend, LPN, and another mesh device (third node) that becomes the controlled
# source of the retransmitted group PDU.
#
# Topology (all nodes in mutual radio range):
#
#              Other (net-retransmit burst to group after LPN trigger)
#                    |
#    Friend ←→ LPN   |
#
# Test procedure (high level):
# 1. Friend enables the Friend feature and waits for friendship establishment.
# 2. LPN subscribes to a group address, enables LPN, and waits for establishment.
# 3. LPN clears the test message queue, sends a small trigger unicast to the third node, and
#    immediately calls bt_mesh_lpn_poll() to open the receive window.
# 4. Third node waits for the trigger, configures net transmit (count/interval) to stretch
#    retransmissions of one group send, then sends exactly one group message.
# 5. LPN receives the group message (source and destination checked), then verifies no retry polls
#    occurred and the friendship did not drop.

RunTest mesh_friendship_rx_window_replay_collision \
	friendship_lpn_rx_window_replay_collision \
	friendship_other_rx_window_replay_collision \
	friendship_friend_rx_window_guard
