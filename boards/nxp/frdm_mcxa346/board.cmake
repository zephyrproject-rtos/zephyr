#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MCXA346")
board_runner_args(linkserver "--device=MCXA346:FRDM-MCXA346")
board_runner_args(pyocd "--target=MCXA346")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
