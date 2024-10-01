#
# Copyright 2017, 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
board_runner_args(jlink "--device=MCIMXRT1052")
board_runner_args(linkserver "--device=MIMXRT1052xxxxB:EVKB-IMXRT1050")

if("${BOARD_REVISION}" STREQUAL "qspi")
  board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=QSPI")
  board_runner_args(pyocd "--target=mimxrt1050_quadspi")
  board_runner_args(linkserver "--override=/device/memory/3/flash-driver=MIMXRT1050_SFDP_QSPI.cfx")
else()
  board_runner_args(pyocd "--target=mimxrt1050_hyperflash")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
