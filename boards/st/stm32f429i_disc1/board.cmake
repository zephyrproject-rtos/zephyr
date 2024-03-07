# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--erase" "--port=swd" "--reset-mode=hw")
board_runner_args(jlink "--device=STM32F429ZI" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
