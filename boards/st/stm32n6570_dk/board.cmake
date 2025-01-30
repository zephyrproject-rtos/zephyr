# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 STMicroelectronics

board_runner_args(stm32cubeprogrammer "--port=swd")
board_runner_args(stm32cubeprogrammer "--tool-opt= mode=HOTPLUG ap=1")
board_runner_args(stm32cubeprogrammer "--extload=MX66UW1G45G_STM32N6570-DK.stldr")
board_runner_args(stm32cubeprogrammer "--download-address=0x70000000")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
