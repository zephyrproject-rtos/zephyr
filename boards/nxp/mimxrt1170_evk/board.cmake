#
# Copyright (c) 2021, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT1176_CM7 OR CONFIG_SECOND_CORE_MCUX)
 board_runner_args(pyocd "--target=mimxrt1170_cm7")
 board_runner_args(jlink "--device=MIMXRT1176xxxA_M7")

 if(${BOARD_REVISION} STREQUAL "A")
  board_runner_args(linkserver  "--device=MIMXRT1176xxxxx:MIMXRT1170-EVK")
 elseif(${BOARD_REVISION} STREQUAL "B")
  board_runner_args(linkserver "--device=MIMXRT1176xxxxx:MIMXRT1170-EVKB")
 endif()

 board_runner_args(linkserver "--core=cm7")
elseif(CONFIG_SOC_MIMXRT1176_CM4)
 board_runner_args(pyocd "--target=mimxrt1170_cm4")
 # Note: Please use JLINK above V7.50 (Only support run cm4 image when debugging due to default boot core on board is cm7 core)
 board_runner_args(jlink "--device=MIMXRT1176xxxA_M4")
 if(${BOARD_REVISION} STREQUAL "A")
  board_runner_args(linkserver "--device=MIMXRT1176xxxxx:MIMXRT1170-EVK")
 elseif(${BOARD_REVISION} STREQUAL "B")
  board_runner_args(linkserver "--device=MIMXRT1176xxxxx:MIMXRT1170-EVKB")
 endif()
 board_runner_args(linkserver "--core=cm4")
endif()

if(NOT SYSBUILD)
  board_runner_args(jlink "--no-reset")
  board_runner_args(linkserver "--no-reset")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
