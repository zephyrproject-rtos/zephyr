#
# Copyright (c) 2024, Toradex
#
# SPDX-License-Identifier: Apache-2.0
#

board_set_debugger_ifnset(jlink)
board_set_flasher_ifnset(jlink)

board_runner_args(jlink "--device=MIMX8MD6_M4")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
