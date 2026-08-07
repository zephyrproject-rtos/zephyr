#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test LPN-initiated friendship termination with Friend Clear Confirm.
#
# Devices:
#   A = LPN    (device 1, addr 0x0004)
#   B = Friend (device 0, addr 0x0001)
#
# Scenario:
# 1. A establishes friendship with B.
# 2. A terminates by sending Friend Clear to B.
# 3. B receives Friend Clear, terminates the friendship, and sends
#    a delayed Friend Clear Confirm back to A.
# 4. A receives the Confirm (verified by old_friend == UNASSIGNED).
#    Without the delay, B's Confirm would be transmitted while A is
#    still advertising its Friend Clear retransmissions.
RunTest mesh_friendship_clear_cfm \
	friendship_friend_est_clear \
	friendship_lpn_clear_cfm
