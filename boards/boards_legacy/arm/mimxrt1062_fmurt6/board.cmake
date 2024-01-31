#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2023 NXP
#

board_runner_args(pyocd "--target=mimxrt1060")
board_runner_args(jlink "--device=MIMXRT1062xxx6A")

board_runner_args(jlink "--loader=BankAddr=0x60000000&Loader=HyperFlash")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
