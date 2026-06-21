# Copyright (c) 2024 Kenneth Lu <kuohsianglu@gmail.com>
# Copyright (c) 2026 JaeHwan Jin <jaehwan.jin@rakwireless.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=stm32wle5ccux")
board_runner_args(pyocd "--flash-opt=-O cmsis_dap.limit_packets=1")
board_runner_args(jlink "--device=STM32WLE5CC" "--speed=4000" "--reset-after-load")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

zephyr_get(OPENOCD_STM32WL_INTERFACE)
if(NOT DEFINED OPENOCD_STM32WL_INTERFACE)
  set(OPENOCD_STM32WL_INTERFACE "cmsis-dap")
endif()
board_runner_args(openocd --cmd-pre-init "source [find interface/${OPENOCD_STM32WL_INTERFACE}.cfg]")
board_runner_args(openocd --cmd-pre-init "transport select swd")
board_runner_args(openocd --cmd-pre-init "source [find target/stm32wlx.cfg]")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
