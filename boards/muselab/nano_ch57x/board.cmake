# SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
# SPDX-License-Identifier: Apache-2.0

board_runner_args(OPEN "--file-type=elf" "--cmd-reset-halt" "halt")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

find_program(OPENOCD openocd)
set(OPENOCD ${OPENOCD} CACHE STRING "" FORCE)
