# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MK82FN256xxx15")
board_runner_args(pyocd "--target=k82f25615")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
