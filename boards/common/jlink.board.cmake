# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(jlink)
board_set_debugger_ifnset(jlink)
board_finalize_runner_args(jlink "--dt-flash=y")
