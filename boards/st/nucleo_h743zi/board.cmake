# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-post-verify "reset halt")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
board_runner_args(jlink "--device=STM32H743ZI" "--speed=4000")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
