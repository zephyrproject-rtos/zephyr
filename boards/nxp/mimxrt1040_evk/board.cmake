#
# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MIMXRT1042XXX6B")
board_runner_args(linkserver "--device=MIMXRT1042xxxxB:MIMXRT1040-EVK")
board_runner_args(pyocd "--target=MIMXRT1042XJM5B")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
