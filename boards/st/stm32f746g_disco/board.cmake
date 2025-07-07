# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
if(CONFIG_XIP AND (CONFIG_STM32_MEMMAP OR CONFIG_BOOTLOADER_MCUBOOT))
  board_runner_args(stm32cubeprogrammer "--extload=N25Q128A_STM32F746G-DISCO.stldr")
endif()

board_runner_args(jlink "--device=STM32F746NG" "--speed=4000")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
