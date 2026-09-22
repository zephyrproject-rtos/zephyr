# Copyright 2026 u-blox
# SPDX-License-Identifier: Apache-2.0

board_runner_args(linkserver "--device=MCXW716CxxxA:UBX_EVKNINAB5")
board_runner_args(jlink "--device=mcxw716" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
