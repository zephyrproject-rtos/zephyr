# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_ARDUINO_GIGA_R1_STM32H747XX_M7)
board_runner_args(jlink "--device=STM32H747XI_M7" "--speed=4000")
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_arduino_giga_r1_m7.cfg")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_ARDUINO_GIGA_R1_STM32H747XX_M4)
board_runner_args(jlink "--device=STM32H747XI_M4" "--speed=4000")
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_arduino_giga_r1_m4.cfg")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()
board_runner_args(dfu-util "--pid=2341:0366" "--alt=0" "--dfuse")
board_runner_args(blackmagicprobe "--connect-rst")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
