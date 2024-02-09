#
# Copyright 2018, 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(pyocd "--target=mimxrt1064")
board_runner_args(jlink "--device=MIMXRT1064")
board_runner_args(linkserver  "--device=MIMXRT1064xxxxA:EVK-MIMXRT1064")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
