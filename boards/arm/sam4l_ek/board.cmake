# Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=atsam4lc4c")
board_runner_args(jlink "--speed=4000")
board_runner_args(jlink "--reset-after-load")
board_runner_args(jlink "-rtos GDBServer/RTOSPlugin_Zephyr")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
