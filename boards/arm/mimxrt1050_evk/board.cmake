#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
board_runner_args(jlink "--device=MCIMXRT1052")

if(${CONFIG_BOARD_MIMXRT1050_EVK_QSPI})
    board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=QSPI")
    board_runner_args(pyocd "--target=mimxrt1050_quadspi")
else()
    board_runner_args(pyocd "--target=mimxrt1050_hyperflash")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
