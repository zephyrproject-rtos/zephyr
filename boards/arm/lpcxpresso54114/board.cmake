#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

set_ifndef(LPCLINK_FW jlink)

if(LPCLINK_FW STREQUAL jlink)
  board_set_debugger_ifnset(jlink)
  board_set_flasher_ifnset(jlink)
endif()

if(CONFIG_BOARD_LPCXPRESSO54114_M4)
board_runner_args(jlink "--device=LPC54114J256_M4" "--reset-after-load")
elseif(CONFIG_BOARD_LPCXPRESSO54114_M0)
board_runner_args(jlink "--device=LPC54114J256_M0" "--reset-after-load")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
