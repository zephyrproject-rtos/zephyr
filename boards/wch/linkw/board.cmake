# Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
# SPDX-License-Identifier: Apache-2.0

board_runner_args(minichlink)
board_runner_args(wlink "--chip=CH32V20X")
board_runner_args(wchisp)
include(${ZEPHYR_BASE}/boards/common/minichlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/wlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/wchisp.board.cmake)

board_runner_args(openocd "--file-type=elf" "--cmd-reset-halt" "halt")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
