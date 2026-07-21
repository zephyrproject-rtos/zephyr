#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(pyocd "--target=mimxrt1020")
board_runner_args(jlink "--device=MIMXRT1021xxx5A")
board_runner_args(linkserver  "--device=MIMXRT1021xxxxx:EVK-MIMXRT1020")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
