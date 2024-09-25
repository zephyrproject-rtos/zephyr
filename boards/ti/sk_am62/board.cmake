# Texas Instruments Sitara AM62x-SK-M4 EVM
#
# Copyright (c) 2024, PHYTEC Messtechnik GmbH
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_AM6234_M4)
  board_runner_args(openocd "--no-init" "--no-halt" "--no-targets" "--gdb-client-port=3339")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
