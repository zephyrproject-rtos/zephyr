# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 STMicroelectronics

if(CONFIG_BOARD_STM32H747I_DISCO_STM32H747XX_M7)
  board_runner_args(jlink "--device=STM32H747ZI_M7")
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32h747i_disco_m7.cfg")
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_STM32H747I_DISCO_STM32H747XX_M4)
  board_runner_args(jlink "--device=STM32H747ZI_M4")
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32h747i_disco_m4.cfg")
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
