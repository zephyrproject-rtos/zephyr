# SPDX-License-Identifier: Apache-2.0

if("${BOARD_QUALIFIERS}" MATCHES "stm32f103x6")
  board_runner_args(jlink "--device=STM32F103C6" "--speed=4000")
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32f103x6.cfg")
else()
  board_runner_args(jlink "--device=STM32F103C8" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
