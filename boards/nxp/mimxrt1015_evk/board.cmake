#
# Copyright (c) 2019, 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(pyocd "--target=mimxrt1015")
board_runner_args(jlink "--device=MIMXRT1015")
board_runner_args(linkserver  "--device=MIMXRT1015xxxxx:EVK-MIMXRT1015")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
