# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(linkserver  "--device=MCXW236:FRDM-MCXW23")
board_runner_args(jlink "--device=MCXW236" "--reset-after-load")
board_runner_args(pyocd "--target=mcxw236")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
