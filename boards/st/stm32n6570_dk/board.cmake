# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 STMicroelectronics

if(CONFIG_STM32N6_BOOT_SERIAL)
  board_runner_args(stm32cubeprogrammer "--port=usb1")
  board_runner_args(stm32cubeprogrammer "--download-modifiers=0x1")
  board_runner_args(stm32cubeprogrammer "--start-modifiers=noack")
else()
  board_runner_args(stm32cubeprogrammer "--port=swd")
  board_runner_args(stm32cubeprogrammer "--tool-opt= mode=HOTPLUG ap=1")
  board_runner_args(stm32cubeprogrammer "--extload=MX66UW1G45G_STM32N6570-DK.stldr")

  if(CONFIG_BOOTLOADER_MCUBOOT)
    dt_chosen(app_partition_node PROPERTY "zephyr,code-partition")
  else()
    dt_chosen(app_partition_node PROPERTY "zephyr,flash")
  endif()
  dt_reg_addr(app_base_addr PATH ${app_partition_node})
  board_runner_args(stm32cubeprogrammer "--download-address=${app_base_addr}")
endif()

board_runner_args(stlink_gdbserver "--apid=1")
board_runner_args(stlink_gdbserver "--extload=MX66UW1G45G_STM32N6570-DK.stldr")


include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stlink_gdbserver.board.cmake)
