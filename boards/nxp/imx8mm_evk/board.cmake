# SPDX-License-Identifier: Apache-2.0
# Copyright 2024 NXP

if(CONFIG_BOARD_IMX8MM_EVK_MIMX8MM6_M4)
  board_set_debugger_ifnset(jlink)
  board_set_flasher_ifnset(jlink)

  board_runner_args(jlink "--device=MIMX8MD6_M4")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
