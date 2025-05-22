# Copyright (c) 2024 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

# By default stm32cubeprogrammer configured to use DFU upload method.
# Comment below line and uncomment the second line to use SWD upoad method.
board_runner_args(stm32cubeprogrammer "--port=usb1")
# board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

# Even if flash and start work, dfu-util return error 74. It can be ignored.
board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
