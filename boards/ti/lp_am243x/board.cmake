# Copyright (c) 2025 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_AM2434_M4)
board_runner_args(openocd "--no-init" "--no-halt" "--no-targets" "--gdb-client-port=3338")
elseif(CONFIG_SOC_AM2434_R5F0_0)
board_runner_args(openocd "--no-init" "--no-halt" "--no-targets" "--gdb-client-port=3334")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)