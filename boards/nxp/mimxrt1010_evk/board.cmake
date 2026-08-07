#
# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(pyocd "--target=mimxrt1010")
board_runner_args(jlink "--device=MIMXRT1011")
board_runner_args(linkserver  "--device=MIMXRT1011xxxxx:EVK-MIMXRT1010")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
