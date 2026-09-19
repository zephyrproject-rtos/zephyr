# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(OPENOCD "--file-type=elf" "--cmd-reset-halt" "halt")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

find_program(OPENOCD openocd)
set(OPENOCD ${OPENOCD} CACHE STRING "" FORCE)
