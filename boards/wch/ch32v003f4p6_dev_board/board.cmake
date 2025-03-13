# Copyright (c) 2025 Andrei-Edward Popa
# SPDX-License-Identifier: Apache-2.0

board_runner_args(minichlink)
include(${ZEPHYR_BASE}/boards/common/minichlink.board.cmake)

board_runner_args(openocd "--use-elf" "--cmd-reset-halt" "halt")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
