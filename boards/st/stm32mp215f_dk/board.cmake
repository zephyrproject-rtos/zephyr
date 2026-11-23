# Copyright (C) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)

if(CONFIG_BOARD_STM32MP215F_DK_STM32MP215FXX_M33)
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32mp215f_dk_m33.cfg")
  board_runner_args(openocd "--gdb-client-port=3334")
endif()
