#
# Copyright 2025 PHYTEC America, LLC
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT1176_CM7 OR CONFIG_SECOND_CORE_MCUX)
  board_runner_args(linkserver "--no-reset")
  board_runner_args(pyocd "--target=mimxrt1170_cm7")
  board_runner_args(jlink "--device=MIMXRT1176xxxA_M7")
  board_runner_args(jlink "--no-reset")

  board_runner_args(linkserver "--device=MIMXRT1176xxxxx:MIMXRT1170-EVK")
  board_runner_args(linkserver "--core=cm7")
elseif(CONFIG_SOC_MIMXRT1176_CM4)
  board_runner_args(pyocd "--target=mimxrt1170_cm4")
  # Note: Use J-Link version 7.50 or later. Debugging only supports running
  # the CM4 image, since the board’s default boot core is CM7.
  board_runner_args(jlink "--device=MIMXRT1176xxxA_M4")

  board_runner_args(linkserver "--device=MIMXRT1176xxxxx:MIMXRT1170-EVK")
  board_runner_args(linkserver "--core=cm4")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
