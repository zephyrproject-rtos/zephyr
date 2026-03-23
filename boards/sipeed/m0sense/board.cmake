# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(bflb_mcu_tool --chipname bl702)
include(${ZEPHYR_BASE}/boards/common/bflb_mcu_tool.board.cmake)

board_set_flasher(bflb_mcu_tool)
