# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX9529_A55)
  board_runner_args(jlink "--device=IMX952_A55" "--no-reset" "--flash-sram")

  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
