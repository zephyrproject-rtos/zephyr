# Copyright (C) 2025 Savoir-faire Linux, Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_STM32MP257F_EV1_STM32MP257FXX_M33)
	board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32mp257f_ev1_m33.cfg")
	board_runner_args(openocd "--gdb-init=target extended-remote :3334")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
