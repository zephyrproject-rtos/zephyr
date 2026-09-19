# BeagleBoard.org Beagle Y-AI
# Copyright (c) Dhruv Menon <dhruvmenon1104@gmail.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_J722S_MAIN_R5F0_0)
  board_runner_args(openocd "--gdb-client-port=3339" "--target-handle=j722s.cpu.main0_r5.0")
elseif(CONFIG_SOC_J722S_MCU_R5F0_0)
  board_runner_args(openocd "--gdb-client-port=3340" "--target-handle=j722s.cpu.mcu0_r5.0")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
