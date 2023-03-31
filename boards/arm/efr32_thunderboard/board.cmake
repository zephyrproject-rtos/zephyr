# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_EFR32BG22_BRD4184A)
board_runner_args(jlink "--device=EFR32BG22C224F512IM40" "--reset-after-load")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

elseif(CONFIG_BOARD_EFR32BG27_BRD2602A)
board_runner_args(silabs_commander "--device=EFR32BG27C140F768IM40")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)

endif()
