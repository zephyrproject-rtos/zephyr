# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>

board_runner_args(jlink "--device=STM32H745ZI" "--speed=4000")
if(CONFIG_BOARD_NUCLEO_H745ZI_Q_STM32H745XX_M7)
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_NUCLEO_H745ZI_Q_STM32H745XX_M4)
board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
