# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(pyocd "--target=EFR32MG24B220F1536IM48")
board_runner_args(pyocd "--tool-opt=-Oadi.v5.max_invalid_ap_count=0")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)

board_runner_args(jlink "--device=EFR32MG24BxxxF1536")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
