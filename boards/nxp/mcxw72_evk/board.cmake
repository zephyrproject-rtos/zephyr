# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MCXW727" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
