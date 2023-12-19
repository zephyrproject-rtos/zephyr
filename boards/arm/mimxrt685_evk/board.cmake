#
# Copyright (c) 2020, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MIMXRT685S_M33" "--reset-after-load")
board_runner_args(linkserver  "--device=MIMXRT685S:EVK-MIMXRT685")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
