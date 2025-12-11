# Copyright (c) 2024 Ian Morris
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=stm32f415rg")
board_runner_args(jlink "--device=STM32F415RG" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
