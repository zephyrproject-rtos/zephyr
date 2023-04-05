# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_EFR32BG_SLTB010A)
board_runner_args(jlink "--device=EFR32BG22C224F512IM40" "--reset-after-load")
endif() # CONFIG_BOARD_EFR32BG_SLTB010A

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
