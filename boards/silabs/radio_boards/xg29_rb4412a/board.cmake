# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFR32MG29BxxxF1024")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=EFR32MG29B140F1024IM40")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
