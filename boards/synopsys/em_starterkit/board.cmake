# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2017 Synopsys

board_runner_args(openocd "--use-elf")
board_runner_args(mdb-hw "--jtag=digilent")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/mdb-hw.board.cmake)
