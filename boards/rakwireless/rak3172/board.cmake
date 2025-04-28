# Copyright (c) 2024 Kenneth Lu <kuohsianglu@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=stm32wle5ccux")
board_runner_args(pyocd "--flash-opt=-O cmsis_dap.limit_packets=1")
board_runner_args(jlink "--device=STM32WLE5CC" "--speed=4000" "--reset-after-load")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
