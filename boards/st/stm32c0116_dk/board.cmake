# Copyright (c) 2024 Kickmaker
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32C011F6" "--speed=4000")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
