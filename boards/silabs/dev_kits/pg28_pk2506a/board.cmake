# Copyright (c) 2025 Christoph Jans
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFM32PG28BxxxF1024")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(openocd)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(silabs_commander "--device=EFM32PG28BxxxF1024")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
