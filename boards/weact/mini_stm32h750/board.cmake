# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
if(CONFIG_FLASH_STM32_NOR_MEMMAP OR CONFIG_BOOTLOADER_MCUBOOT)
  board_runner_args(stm32cubeprogrammer "--extload=STM32H7xx_W25Q128_WeActStudio.stldr")
endif()

board_runner_args(stlink_gdbserver "--extload=STM32H7xx_W25Q128_WeActStudio.stldr")
board_runner_args(jlink "--device=STM32H750VB" "--speed=4000")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stlink_gdbserver.board.cmake)
