# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MKW40Z160xxx4")
board_runner_args(pyocd "--target=kw40z4")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
