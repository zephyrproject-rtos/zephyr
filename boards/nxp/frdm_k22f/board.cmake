# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MK22FN512xxx12")
board_runner_args(pyocd "--target=k22f")
board_runner_args(linkserver  "--device=MK22FN512xxx12:FRDM-K22F")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
