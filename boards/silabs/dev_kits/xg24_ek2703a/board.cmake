# Copyright (c) 2021, Sateesh Kotapati
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFR32MG24BxxxF1536" "--reset-after-load")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=EFR32MG24B210F1536IM48")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
