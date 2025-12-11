# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MCXE31B")
board_runner_args(linkserver "--device=MCXE31B:FRDM-MCXE31B")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
