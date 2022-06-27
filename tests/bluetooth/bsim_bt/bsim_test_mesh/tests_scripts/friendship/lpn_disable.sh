#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Check that disabling LPN gives correct behaviour.
#
# In this test the lpn node will enable the lpn feature, and then immediately
# disables it again. Then we check that that the lpn node is actually in a
# disabled state. This test ensures that a lpn disable call is not overwritten
# by a subsequent lpn enable call, since the enable call is associated with
# internal callback structures that might produce incorrect internal state
# of the LPN module
RunTest mesh_lpn_disable_check \
	friendship_friend_no_est \
	friendship_lpn_disable
