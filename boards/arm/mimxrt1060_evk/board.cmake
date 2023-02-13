#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(pyocd "--target=mimxrt1060")
board_runner_args(jlink "--device=MIMXRT1062xxx6A")

if ((${CONFIG_BOARD_MIMXRT1060_EVK}) OR (${CONFIG_BOARD_MIMXRT1060_EVKB}))
    board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=QSPI")
elseif (${CONFIG_BOARD_MIMXRT1060_EVK_HYPERFLASH})
    board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=HyperFlash")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
