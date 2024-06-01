# Copyright (c) 2024 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R7FA2A1AB")

board_runner_args(pyocd "--target=r7fa2a1ab")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
