# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32F429II" "--speed=4000")
board_runner_args(pyocd "--target=stm32f429xi")
board_runner_args(pyocd "--flash-opt=-O reset_type=hw")
board_runner_args(pyocd "--flash-opt=-O connect_mode=under-reset")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
