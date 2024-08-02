# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32H735IG" "--speed=4000")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

if(CONFIG_STM32_MEMMAP)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(stm32cubeprogrammer "--extload=MT25TL01G_STM32H750B-DISCO.stldr")
else()
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw" )
endif()

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
