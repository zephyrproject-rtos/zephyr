# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_XIP)
  board_runner_args(openocd "--use-elf")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
