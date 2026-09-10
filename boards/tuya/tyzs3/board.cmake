# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFR32MG13PxxxF512")
board_runner_args(openocd)
board_runner_args(pyocd "--target=efr32mg13p732f512gm48")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
