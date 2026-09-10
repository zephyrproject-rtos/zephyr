#
# Copyright (c) 2021, Kwon Tae-young <tykwon@m2i.co.kr>
#
# SPDX-License-Identifier: Apache-2.0
#

board_set_debugger_ifnset(jlink)
board_set_flasher_ifnset(jlink)

board_runner_args(jlink "--device=MIMX8MQ6_M4")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
