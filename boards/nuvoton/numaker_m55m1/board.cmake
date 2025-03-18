# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=m55m1h2ljae")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
