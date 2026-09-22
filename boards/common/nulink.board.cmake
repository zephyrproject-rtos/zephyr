# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(nulink)
board_set_debugger_ifnset(nulink)
board_finalize_runner_args(nulink "-f")
