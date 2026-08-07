# SPDX-License-Identifier: Apache-2.0
board_runner_args(openocd "--file-type=elf")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
