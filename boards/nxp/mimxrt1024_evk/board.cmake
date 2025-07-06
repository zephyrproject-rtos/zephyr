#
# Copyright (c) 2020, 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(pyocd "--target=mimxrt1024")
board_runner_args(jlink "--device=MIMXRT1024xxx5A")
# MIMXRT1024xxxxx         MIMXRT1024-EVK           cm7
board_runner_args(linkserver  "--device=MIMXRT1024xxxxx:MIMXRT1024-EVK")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
