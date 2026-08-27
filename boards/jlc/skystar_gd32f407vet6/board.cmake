# Copyright (c) 2025, Liu Changjie <liucj1228@outlook.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=gd32f407ve")
board_runner_args(jlink "--device=GD32F407VE" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
