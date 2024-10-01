# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(linkserver)
board_set_debugger_ifnset(linkserver)
board_finalize_runner_args(linkserver "--dt-flash=y")
