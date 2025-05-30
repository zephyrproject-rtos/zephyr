# Copyright 2025 Variscite Ltd.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX9352_M33)
  board_set_debugger_ifnset(jlink)
  board_set_flasher_ifnset(jlink)

  board_runner_args(jlink "--device=MIMX9352_M33")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
