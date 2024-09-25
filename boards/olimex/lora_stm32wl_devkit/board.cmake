# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=stm32wle5ccux")
board_runner_args(pyocd "--flash-opt=-O cmsis_dap.limit_packets=1")
board_runner_args(jlink "--device=STM32WLE5CC" "--speed=4000" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

# OpenOCD not enabled because it failed with parameters suggested here:
# https://github.com/OLIMEX/LoRa-STM32WL-DevKIT/blob/main/DOCUMENTS/STM32CubeIDE%20-%20How%20to%20use%20OpenOCD
