# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 STMicroelectronics

if(CONFIG_STM32N6_BOOT_SERIAL)
  board_runner_args(stm32cubeprogrammer "--port=usb1")
  board_runner_args(stm32cubeprogrammer "--download-modifiers=0x1")
  board_runner_args(stm32cubeprogrammer "--start-modifiers=noack")
else()
  board_runner_args(stm32cubeprogrammer "--port=swd")
  board_runner_args(stm32cubeprogrammer "--tool-opt= mode=HOTPLUG ap=1")
  board_runner_args(stm32cubeprogrammer "--extload=MX25UM51245G_STM32N6570-NUCLEO.stldr")
  board_runner_args(stm32cubeprogrammer "--download-address=0x70000000")
endif()

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
