# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--cmd-pre-load=reset init")
# unprotect sectors before running
board_runner_args(openocd "--cmd-pre-load=flash protect 1 0 last off")
board_runner_args(openocd "--cmd-pre-load=flash erase_sector 1 0 last")

#board_runner_args(openocd "--cmd-reset-halt=flash info 1")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
