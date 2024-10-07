#
# Copyright 2017, 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(NOT ("${BOARD_QUALIFIERS}" MATCHES "qspi"
      OR "${BOARD_QUALIFIERS}" MATCHES "hyperflash"))
  message(FATAL_ERROR "Please specify a board flash variant for the mimxrt1050_evk:\n"
                      "mimxrt1050_evk/mimxrt1052/qspi or mimxrt1050_evk/mimxrt1052/hyperflash\n")
endif()

board_runner_args(jlink "--device=MCIMXRT1052")
board_runner_args(linkserver "--device=MIMXRT1052xxxxB:EVKB-IMXRT1050")

if("${BOARD_QUALIFIERS}" MATCHES "qspi")
  board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=QSPI")
  board_runner_args(pyocd "--target=mimxrt1050_quadspi")
  board_runner_args(linkserver "--override=/device/memory/3/flash-driver=MIMXRT1050_SFDP_QSPI.cfx")
else()
  board_runner_args(pyocd "--target=mimxrt1050_hyperflash")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
