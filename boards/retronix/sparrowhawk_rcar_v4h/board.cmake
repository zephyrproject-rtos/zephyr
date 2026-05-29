# SPDX-License-Identifier: Apache-2.0
if(CONFIG_BOARD_SPARROWHAWK_RCAR_V4H_R8A779G0_R52)
  board_runner_args(openocd "--use-elf")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
