#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MCXA166")
board_runner_args(linkserver "--device=MCXA166:FRDM-MCXA166")
board_runner_args(pyocd "--target=mcxA166")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
