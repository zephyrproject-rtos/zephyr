# Copyright (C) 2026 DH electronics GmbH
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_STM32MP255C_DHSBC_STM32MP255CXX_M33)
  board_runner_args(jlink "--device=STM32MP255C_M33" "--speed=4000")
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32mp255c_dhsbc_m33.cfg")
  board_runner_args(openocd "--gdb-init=target extended-remote :3334")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
