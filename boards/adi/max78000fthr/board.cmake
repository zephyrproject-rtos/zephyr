# Copyright (c) 2025 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-pre-init "source [find interface/cmsis-dap.cfg]")
board_runner_args(openocd --cmd-pre-init "source [find target/max78000.cfg]")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
