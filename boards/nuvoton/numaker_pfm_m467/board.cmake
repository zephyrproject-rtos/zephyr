# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=m467hjhae")
board_runner_args(nulink "-f")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nulink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/canopen.board.cmake)
