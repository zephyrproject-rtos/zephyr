# SPDX-License-Identifier: Apache-2.0

board_runner_args(nulink "-f")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nulink.board.cmake)
