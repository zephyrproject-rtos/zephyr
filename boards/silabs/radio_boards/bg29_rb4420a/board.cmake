# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFR32BG29BxxxF1024")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=EFR32BG29B220F1024CJ45")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
