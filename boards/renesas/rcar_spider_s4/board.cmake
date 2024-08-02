# SPDX-License-Identifier: Apache-2.0
if(CONFIG_BOARD_RCAR_SPIDER_S4_R8A779F0_R52)
  board_runner_args(openocd "--use-elf")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
