#
# Copyright (c) 2021, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(pyocd "--target=mimxrt1160_cm7")
board_runner_args(jlink "--device=MIMXRT1166xxx6_M7" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
