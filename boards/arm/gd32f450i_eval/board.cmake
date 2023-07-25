# Copyright (c) 2021, Teslabs Engineering S.L.
# Copyright (c) 2021, BrainCo Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=GD32F450IK" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
