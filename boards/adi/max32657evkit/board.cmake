# Copyright (c) 2024-2025 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-pre-init "source [find interface/jlink.cfg]")
board_runner_args(openocd --cmd-pre-init "source [find target/max32657.cfg]")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
