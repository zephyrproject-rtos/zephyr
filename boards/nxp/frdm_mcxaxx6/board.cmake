#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MCXA266)
  board_runner_args(jlink "--device=MCXA266")
  board_runner_args(linkserver "--device=MCXA266:FRDM-MCXA266")
  board_runner_args(pyocd "--target=mcxa266")
elseif(CONFIG_SOC_MCXA346)
  board_runner_args(jlink "--device=MCXA346")
  board_runner_args(linkserver "--device=MCXA346:FRDM-MCXA346")
  board_runner_args(pyocd "--target=mcxa346")
elseif(CONFIG_SOC_MCXA366)
  board_runner_args(jlink "--device=MCXA366")
  board_runner_args(linkserver "--device=MCXA366:FRDM-MCXA366")
  board_runner_args(pyocd "--target=mcxa366")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
