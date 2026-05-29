# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MCXL255_CPU0 OR CONFIG_SECOND_CORE_MCUX)
  board_runner_args(jlink "--device=MCXL255_M33" "--reset-after-load")
  board_runner_args(linkserver  "--device=MCXL255:FRDM-MCXL255")
  if(CONFIG_SECOND_CORE_MCUX)
    board_runner_args(linkserver  "--core=all")
  else()
    board_runner_args(linkserver  "--core=cm33")
  endif()
  board_runner_args(linkserver  "--override=/device/memory/1/flash-driver=MCXL25x_S.cfx")
  board_runner_args(linkserver  "--override=/device/memory/1/location=0x10000000")
elseif(CONFIG_SOC_MCXL255_CPU1)
  board_runner_args(jlink "--device=MCXL255_M0P" "--reset-after-load")
  board_runner_args(linkserver  "--device=MCXL255:FRDM-MCXL255")
  board_runner_args(linkserver  "--core=cm0plus")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
