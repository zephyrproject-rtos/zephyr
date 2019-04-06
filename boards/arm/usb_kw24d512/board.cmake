# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MKW24D512xxx5" "--speed=4000")
board_runner_args(pyocd "--target=kw24d5")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
