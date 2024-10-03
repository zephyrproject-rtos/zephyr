#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT798S_CPU0)
  board_runner_args(jlink "--device=MIMXRT798S_M33_0" "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT798S:MIMXRT700-EVK")
  board_runner_args(linkserver  "--core=cm33_core0")
  board_runner_args(linkserver  "--override=/device/memory/1/flash-driver=MIMXRT700_XSPI0_Octal.cfx")
  board_runner_args(linkserver  "--override=/device/memory/1/location=0x38000000")
elseif(CONFIG_SOC_MIMXRT798S_CPU1)
  board_runner_args(jlink "--device=MIMXRT798S_M33_1")
  board_runner_args(linkserver  "--device=MIMXRT798S:MIMXRT700-EVK")
  board_runner_args(linkserver  "--core=cm33_core1")         
else()
  message(FATAL_ERROR "Support for cpu1 not available yet")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
