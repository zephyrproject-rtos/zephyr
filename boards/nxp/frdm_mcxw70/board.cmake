# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(linkserver "--device=MCXW70AC:FRDM-MCXW70")
board_runner_args(jlink "--device=MCXW70AC" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
