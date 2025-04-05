# SPDX-License-Identifier: Apache-2.0

# keep first
if(CONFIG_STM32_MEMMAP)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(stm32cubeprogrammer "--extload=MX25UW25645G_NUCLEO-H7S3L8.stldr")
else()
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw" )
endif()

board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
