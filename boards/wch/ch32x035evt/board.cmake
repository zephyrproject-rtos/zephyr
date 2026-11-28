# Copyright (c) 2026 Chen Ren
# SPDX-License-Identifier: Apache-2.0

board_runner_args(minichlink)
board_runner_args(wlink "--chip=CH32X03X")
include(${ZEPHYR_BASE}/boards/common/minichlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/wlink.board.cmake)

board_runner_args(openocd "--file-type=elf" "--cmd-reset-halt" "halt")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
