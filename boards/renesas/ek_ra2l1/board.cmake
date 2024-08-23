# Copyright (c) 2021-2024 MUNIC SA
# SPDX-License-Identifier: Apache-2.0

board_set_debugger_ifnset(jlink)
board_set_flasher_ifnset(jlink)

board_runner_args(jlink "--device=r7fa2l1ab")
board_runner_args(pyocd "--target=r7fa2l1ab")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
