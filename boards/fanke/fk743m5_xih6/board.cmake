# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(jlink "--device=STM32H743XI" "--speed=4000")
board_runner_args(pyocd "--target=STM32H743XIHx")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)