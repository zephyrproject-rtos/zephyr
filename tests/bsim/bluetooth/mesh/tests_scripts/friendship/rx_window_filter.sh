#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Reproduce friendship receive-window filtering race:
# - LPN and Friend establish friendship.
# - A third node keeps sending unicast packets to the LPN during early polls.
# - LPN shall still establish and keep friendship.
RunTest mesh_friendship_rx_window_filter \
	friendship_lpn_rx_window_filter \
	friendship_other_rx_window_filter \
	friendship_friend_rx_window_guard

# Verify filtering in receive windows after initial friendship establishment.
RunTest mesh_friendship_rx_window_filter_post_est \
	friendship_lpn_rx_window_filter_post_est \
	friendship_other_rx_window_filter_post_est \
	friendship_friend_rx_window_guard

# Deterministic replay-collision scenario:
# one third-node message is retransmitted while LPN waits for Friend response.
RunTest mesh_friendship_rx_window_replay_collision \
	friendship_lpn_rx_window_replay_collision \
	friendship_other_rx_window_replay_collision \
	friendship_friend_rx_window_guard
