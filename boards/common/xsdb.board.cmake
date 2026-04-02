# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
board_set_debugger_ifnset(xsdb)
board_set_flasher_ifnset(xsdb)
board_finalize_runner_args(xsdb)
