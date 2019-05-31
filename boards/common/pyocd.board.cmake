# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(pyocd)
board_set_debugger_ifnset(pyocd)
board_finalize_runner_args(pyocd "--dt-flash=y")
