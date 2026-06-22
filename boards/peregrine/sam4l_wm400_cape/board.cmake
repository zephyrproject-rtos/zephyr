# Copyright (c) 2020-2025 Gerson Fernando Budke <nandojve@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=atsam4lc4b")
board_runner_args(jlink "--speed=4000")
board_runner_args(jlink "--reset-after-load")
board_runner_args(jlink "-rtos GDBServer/RTOSPlugin_Zephyr")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(bossac "--bossac-port=/dev/ttyACM0")
board_runner_args(bossac "--erase")
include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
