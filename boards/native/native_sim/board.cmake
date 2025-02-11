# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS native)

board_set_debugger_ifnset(native)
board_set_flasher_ifnset(native)
board_finalize_runner_args(native)
