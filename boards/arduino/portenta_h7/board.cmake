# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_ARDUINO_PORTENTA_H7_STM32H747XX_M7)
board_runner_args(jlink "--device=STM32H747XI_M7" "--speed=4000")
elseif(CONFIG_BOARD_ARDUINO_PORTENTA_H7_STM32H747XX_M4)
board_runner_args(jlink "--device=STM32H747XI_M4" "--speed=4000")
endif()
board_runner_args(dfu-util "--pid=2341:035b" "--alt=0" "--dfuse")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
