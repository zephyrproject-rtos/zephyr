# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Schlumberger

board_set_debugger_ifnset(jlink)
board_set_flasher_ifnset(jlink)

board_runner_args(jlink "--device=XMC4700-2048")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
