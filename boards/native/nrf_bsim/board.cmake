# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2023 Nordic Semiconductor ASA

set(SUPPORTED_EMU_PLATFORMS native)

board_set_debugger_ifnset(native_gdb)
board_finalize_runner_args(native_gdb)
