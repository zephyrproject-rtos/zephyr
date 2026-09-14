# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFR32BG21BxxxF1024")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=EFR32BG21B010F1024IM32")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
