# Copyright (c) 2025 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
if(CONFIG_BUILD_WITH_TFM)
  board_runner_args(stm32cubeprogrammer "--erase")
endif()

# Keep OpenOCD from issuing a startup halt command. Debug still halts via
# board openocd.cfg init hook, while attach can connect to a running target.
board_runner_args(openocd "--no-halt")

board_runner_args(pyocd
  "--target=STM32WBA65RIVx"
  "--frequency=1000000"
  "--no-load"
  "--tool-opt=-M=attach"
  "--tool-opt=--script=${BOARD_DIR}/support/pyocd_user.py"
)

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/st/common/common.cmake)
