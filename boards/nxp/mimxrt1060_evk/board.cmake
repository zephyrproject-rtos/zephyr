#
# Copyright (c) 2018, 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(NOT ("${BOARD_QUALIFIERS}" MATCHES "qspi"
      OR "${BOARD_QUALIFIERS}" MATCHES "hyperflash"))
  message(FATAL_ERROR "Please specify a board flash variant for the mimxrt1060_evk:\n"
                      "mimxrt1060_evk/mimxrt1062/qspi or mimxrt1060_evk/mimxrt1062/hyperflash\n")
endif()

board_runner_args(pyocd "--target=mimxrt1060")
board_runner_args(jlink "--device=MIMXRT1062xxx6A")

if ("${BOARD_REVISION}" STREQUAL "B")
board_runner_args(linkserver  "--device=MIMXRT1062xxxxB:MIMXRT1060-EVKB")
elseif ("${BOARD_REVISION}" STREQUAL "C")
board_runner_args(linkserver  "--device=MIMXRT1062xxxxB:MIMXRT1060-EVKC")
else()
board_runner_args(linkserver  "--device=MIMXRT1062xxxxA:EVK-MIMXRT1060")
endif()

if(("${BOARD_QUALIFIERS}" MATCHES "qspi") OR ("${BOARD_REVISION}" STREQUAL "B") OR ("${BOARD_REVISION}" STREQUAL "C"))
  board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=QSPI")
elseif ("${BOARD_QUALIFIERS}" MATCHES "hyperflash")
  board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=HyperFlash")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
