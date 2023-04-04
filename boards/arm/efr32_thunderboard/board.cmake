# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_EFR32BG22_BRD4184A)
board_runner_args(jlink "--device=EFR32BG22C224F512IM40" "--reset-after-load")
endif() # CONFIG_BOARD_EFR32BG22_BRD4184A

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
