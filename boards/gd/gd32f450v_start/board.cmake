# Copyright (c) 2022, Teslabs Engineering S.L.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=GD32F450VK" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
