# Copyright (c) 2022, TOKITA Hiroshi <tokita.hiroshi@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=gd32f407vk")
board_runner_args(jlink "--device=GD32F407VK" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
