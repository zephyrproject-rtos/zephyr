# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MCXE32B")
board_runner_args(linkserver "--device=MCXE32B:FRDM-MCXE32B")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
