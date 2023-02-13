# SPDX-License-Identifier: Apache-2.0

board_set_debugger_ifnset(jlink)
board_set_flasher_ifnset(jlink)

board_runner_args(jlink "--device=MK66FN2M0xxx18")
board_runner_args(pyocd "--target=k66f")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
