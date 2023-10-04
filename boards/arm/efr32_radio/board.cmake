# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd)

if(CONFIG_BOARD_EFR32_RADIO_BRD4104A)
board_runner_args(jlink "--device=EFR32BG13PxxxF512")
elseif(CONFIG_BOARD_EFR32_RADIO_BRD4250B)
board_runner_args(jlink "--device=EFR32FG1PxxxF256")
elseif(CONFIG_BOARD_EFR32_RADIO_BRD4180A)
board_runner_args(jlink "--device=EFR32MG21AxxxF1024")
elseif(CONFIG_BOARD_EFR32_RADIO_BRD4187C)
board_runner_args(jlink "--device=EFR32MG24BxxxF1536")
elseif(CONFIG_BOARD_EFR32_RADIO_BRD4255A)
board_runner_args(jlink "--device=EFR32FG13PxxxF512")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
