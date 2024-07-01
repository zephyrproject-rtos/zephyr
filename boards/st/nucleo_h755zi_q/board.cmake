# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(jlink "--device=STM32H755ZI" "--speed=3300")
if(CONFIG_BOARD_NUCLEO_H755ZI_Q_STM32H755XX_M7)
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_NUCLEO_H755ZI_Q_STM32H755XX_M4)
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
