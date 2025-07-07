# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFR32MG22C224F512IM40" "--reset-after-load")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=EFR32MG22E224F512IM40")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
