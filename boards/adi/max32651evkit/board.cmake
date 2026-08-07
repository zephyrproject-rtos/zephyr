# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MAX32651" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/openocd-adi-max32.boards.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
