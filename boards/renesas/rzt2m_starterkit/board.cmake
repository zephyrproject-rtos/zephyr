#
# Copyright (c) 2023 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0
#

board_set_debugger_ifnset(jlink)
board_set_flasher_ifnset(jlink)

board_runner_args(jlink "--device=R9A07G075M2")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
