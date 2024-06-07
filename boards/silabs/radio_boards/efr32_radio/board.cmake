# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd)

if(CONFIG_BOARD_EFR32_RADIO_EFR32BG13P632F512GM48)
  board_runner_args(jlink "--device=EFR32BG13PxxxF512")
elseif(CONFIG_BOARD_EFR32_RADIO_EFR32FG1P133F256GM48)
  board_runner_args(jlink "--device=EFR32FG1PxxxF256")
elseif(CONFIG_BOARD_EFR32_RADIO_EFR32MG12P433F1024GM68)
  board_runner_args(jlink "--device=EFR32MG12PxxxF1024")
elseif(CONFIG_BOARD_EFR32_RADIO_EFR32MG12P432F1024GL125)
  board_runner_args(jlink "--device=EFR32MG12PxxxF1024")
elseif(CONFIG_BOARD_EFR32_RADIO_EFR32MG21A020F1024IM32)
  board_runner_args(jlink "--device=EFR32MG21AxxxF1024")
elseif(CONFIG_BOARD_EFR32_RADIO_EFR32MG24B220F1536IM48)
  board_runner_args(jlink "--device=EFR32MG24BxxxF1536")
elseif(CONFIG_BOARD_EFR32_RADIO_EFR32FG13P233F512GM48)
  board_runner_args(jlink "--device=EFR32FG13PxxxF512")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
