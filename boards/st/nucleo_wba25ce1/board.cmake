# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(stlink_gdbserver "--apid=1")

board_runner_args(pyocd
  "--target=STM32WBA25CEUx"
  "--frequency=1000000"
  "--no-load"
  "--tool-opt=-M=attach"
  "--tool-opt=--script=${BOARD_DIR}/support/pyocd_user.py"
)

# keep first
include(${ZEPHYR_BASE}/boards/common/stlink_gdbserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
