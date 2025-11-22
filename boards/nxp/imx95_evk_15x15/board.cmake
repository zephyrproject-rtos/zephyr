# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX9596_A55)
  board_runner_args(jlink "--device=MIMX9596_A55_0" "--no-reset" "--flash-sram")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
