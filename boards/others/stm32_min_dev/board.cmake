# SPDX-License-Identifier: Apache-2.0

if("${BOARD_QUALIFIERS}" MATCHES "stm32f103x6")
  board_runner_args(jlink "--device=STM32F103C6" "--speed=4000")
else()
  board_runner_args(jlink "--device=STM32F103C8" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
