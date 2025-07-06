#
# Copyright 2019, 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#


## DAP Link implementation in pyocd is underway,
## until then jlink can be used or copy image to storage

board_runner_args(linkserver  "--device=LPC55S69:LPCXpresso55S69")

if(CONFIG_SECOND_CORE_MCUX)
  board_runner_args(linkserver  "--core=all")
elseif(CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU0 OR
   CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU0_NS)
  board_runner_args(jlink "--device=LPC55S69_M33_0")
  board_runner_args(linkserver  "--override=/device/memory/0/flash-driver=LPC55xx_S.cfx")
  board_runner_args(linkserver  "--override=/device/memory/0/location=0x10000000")
  board_runner_args(linkserver  "--core=cm33_core0")
elseif(CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU1)
  board_runner_args(jlink "--device=LPC55S69_M33_1")
  board_runner_args(linkserver  "--core=cm33_core1")
endif()

board_runner_args(pyocd "--target=lpc55s69")

if(CONFIG_BUILD_WITH_TFM)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
