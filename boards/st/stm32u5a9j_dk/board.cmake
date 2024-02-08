# Copyright (c) 2023 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--erase" "--port=swd" "--reset-mode=hw")

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
