# Copyright (c) 2022, Teslabs Engineering S.L.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=gd32e507ve")
board_runner_args(jlink "--device=GD32E507VE" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
