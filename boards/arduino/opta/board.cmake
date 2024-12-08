# SPDX-License-Identifier: Apache-2.0

board_runner_args(dfu-util "--pid=2341:0364" "--alt=0" "--dfuse")

if(CONFIG_BOARD_ARDUINO_OPTA_STM32H747XX_M7)
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_opta_stm32h747xx_m7.cfg")
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_ARDUINO_OPTA_STM32H747XX_M4)
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_opta_stm32h747xx_m4.cfg")
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

# Give priority to dfu-util to flash, ST-Link to debug.
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)