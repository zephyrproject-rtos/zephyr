#
# Copyright (c) 2018, 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(pyocd "--target=mimxrt1060")
board_runner_args(jlink "--device=MIMXRT1062xxx6A")
board_runner_args(linkserver  "--device=MIMXRT1062xxxxA:EVK-MIMXRT1060")

if (("${BOARD_REVISION}" STREQUAL "qspi") OR CONFIG_BOARD_MIMXRT1060_EVKB)
  board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=QSPI")
elseif ("${BOARD_REVISION}" STREQUAL "hyperflash")
  board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=HyperFlash")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
