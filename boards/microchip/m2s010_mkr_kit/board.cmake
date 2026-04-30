# SPDX-FileCopyrightText: Copyright Bavariamatic GmbH
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--config=${CMAKE_CURRENT_LIST_DIR}/support/openocd.cfg")
board_set_flasher_ifnset(openocd)
board_set_debugger_ifnset(openocd)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)