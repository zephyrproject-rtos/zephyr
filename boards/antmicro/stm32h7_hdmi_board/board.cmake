# Copyright (c) 2026 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_STM32H7_HDMI_BOARD_STM32H747XX_M7)
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_m7.cfg")
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_STM32H7_HDMI_BOARD_STM32H747XX_M4)
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_m4.cfg")
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()

include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
