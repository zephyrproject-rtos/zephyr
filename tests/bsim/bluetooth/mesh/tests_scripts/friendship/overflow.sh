#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test friend queue overflow
RunTest mesh_friendship_overflow \
	friendship_friend_overflow \
	friendship_lpn_overflow

conf=prj_mesh1d1_conf
RunTest mesh_friendship_overflow_1d1 \
	friendship_friend_overflow \
	friendship_lpn_overflow
