#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Friend Clear Procedure: new Friend (C) clears old Friend (B).
#
# Devices:
#   A = LPN          (device 2, addr 0x0005)
#   B = Old Friend   (device 1, addr 0x0002)
#   C = New Friend   (device 0, addr 0x0001)
#
# Scenario:
# 1. A establishes friendship with B (C is not yet available, delayed 5s)
# 2. A force-disables LPN without sending Friend Clear, so B still
#    considers the friendship active and old_friend is set to B's address
# 3. A re-enables LPN and sends Friend Request with prev_addr=B.
#    C responds first (lower device index) and establishes with A.
# 4. C automatically starts the Friend Clear Procedure: sends Friend
#    Clear (LPN addr + counter) to B.
# 5. B finds the LPN in its slot, terminates the friendship, and
#    responds with Friend Clear Confirm to C.
# 6. C receives the Confirm and completes the clear procedure.
RunTest mesh_friendship_clear_proc \
	friendship_friend_clear_proc \
	friendship_other_clear_proc \
	friendship_lpn_clear_proc
