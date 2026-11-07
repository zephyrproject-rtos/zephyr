# Copyright 2025 Variscite Ltd. SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX9596_M7)
  board_set_debugger_ifnset(jlink)
  board_set_flasher_ifnset(jlink)

  board_runner_args(jlink "--device=MIMX9596_M7" "--no-reset" "--flash-sram")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()

if(CONFIG_SOC_MIMX9596_A55)
  board_runner_args(jlink "--device=MIMX9596_A55_0" "--no-reset" "--flash-sram")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
