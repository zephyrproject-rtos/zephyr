# BeagleBoard.org BeagleBone AI64
# Copyright (c) Dhruv Menon <dhruvmenon1104@gmail.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_J721E_MCU_R5F0_0)
  board_runner_args(openocd "--gdb-client-port=3336" "--target-handle=j721e.cpu.mcu_r5.0")
elseif(CONFIG_SOC_J721E_MCU_R5F0_1)
  board_runner_args(openocd "--gdb-client-port=3337" "--target-handle=j721e.cpu.mcu_r5.1")
elseif(CONFIG_SOC_J721E_MAIN_R5F0_0)
  board_runner_args(openocd "--gdb-client-port=3338" "--target-handle=j721e.cpu.main0_r5.0")
elseif(CONFIG_SOC_J721E_MAIN_R5F0_1)
  board_runner_args(openocd "--gdb-client-port=3339" "--target-handle=j721e.cpu.main0_r5.1")
elseif(CONFIG_SOC_J721E_MAIN_R5F1_0)
  board_runner_args(openocd "--gdb-client-port=3340" "--target-handle=j721e.cpu.main1_r5.0")
elseif(CONFIG_SOC_J721E_MAIN_R5F1_1)
  board_runner_args(openocd "--gdb-client-port=3341" "--target-handle=j721e.cpu.main1_r5.1")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
