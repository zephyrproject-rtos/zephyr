# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MKW41Z512xxx4")
board_runner_args(pyocd "--target=kw41z4")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
