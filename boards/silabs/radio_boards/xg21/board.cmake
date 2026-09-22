# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if("${BOARD}" MATCHES "slwrb4180")
  board_runner_args(jlink "--device=EFR32MG21AxxxF1024")
else()
  board_runner_args(jlink "--device=EFR32MG21BxxxF1024")
endif()
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=${CONFIG_SOC}")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)

board_runner_args(openocd)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
