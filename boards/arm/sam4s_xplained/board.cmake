# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=atsam4s16c" "--speed=4000")
board_runner_args(jlink "-rtos GDBServer/RTOSPlugin_Zephyr")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
