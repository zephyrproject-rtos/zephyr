# SPDX-License-Identifier: Apache-2.0

# keep first
if(CONFIG_STM32_MEMMAP OR (CONFIG_XIP AND CONFIG_BOOTLOADER_MCUBOOT))
  board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
  board_runner_args(stm32cubeprogrammer "--extload=MX66UW1G45G_STM32H7S78-DK.stldr")
else()
  board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw" )
endif()

board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
# FIXME: openocd runner not yet available.
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
