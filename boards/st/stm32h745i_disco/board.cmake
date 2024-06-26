# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
# Copyright (c) 2024 Tomas Jurena <jurena@utb.cz>

board_runner_args(jlink "--device=STM32H745XI" "--speed=4000")
if(CONFIG_BOARD_STM32H745I_DISCO_STM32H745XX_M7)
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_STM32H745I_DISCO_STM32H745XX_M4)
board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()

if(CONFIG_STM32_MEMMAP)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(stm32cubeprogrammer "--extload=MT25TL01G_STM32H745I-DISCO.stldr")
else()
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
endif()

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
