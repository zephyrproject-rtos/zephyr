# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32flash "--baud-rate=115200" "--start-addr=0x08000000")
board_runner_args(jlink "--device=STM32F103RB" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/stm32flash.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
