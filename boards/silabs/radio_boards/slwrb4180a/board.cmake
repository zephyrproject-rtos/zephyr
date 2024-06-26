# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd)
board_runner_args(jlink "--device=EFR32MG21AxxxF1024")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
