# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_STM32H747I_DISCO_M7)
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32h747i_disco_m7.cfg")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_STM32H747I_DISCO_M4)
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32h747i_disco_m4.cfg")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
