# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_RCAR_H3ULCB_R8A77951_R7)
  board_runner_args(openocd "--use-elf")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
