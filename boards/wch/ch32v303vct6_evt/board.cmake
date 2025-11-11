# Copyright (c) 2025 Bootlin
# SPDX-License-Identifier: Apache-2.0

board_runner_args(minichlink "--dt-flash=y")
include(${ZEPHYR_BASE}/boards/common/minichlink.board.cmake)

board_runner_args(openocd "--use-elf" "--cmd-reset-halt" "halt")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
