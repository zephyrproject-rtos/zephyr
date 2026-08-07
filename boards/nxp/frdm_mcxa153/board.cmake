#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MCXA153")
board_runner_args(linkserver "--device=MCXA153:FRDM-MCXA153")
board_runner_args(pyocd "--target=mcxA153")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
