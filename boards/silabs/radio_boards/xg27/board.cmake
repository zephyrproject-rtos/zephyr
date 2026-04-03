# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_XG27_RB4194A)
  board_runner_args(jlink "--device=EFR32MG27CxxxF768")
else()
  board_runner_args(jlink "--device=EFR32BG27CxxxF768")
endif()
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=${CONFIG_SOC}")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
