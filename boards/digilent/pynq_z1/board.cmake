# Copyright (c) 2026 Radu Pirea
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--file-type=elf" "--cmd-reset-halt" "halt")
board_runner_args(openocd --cmd-pre-load "ps7_reset" --cmd-pre-load "ps7_init")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
