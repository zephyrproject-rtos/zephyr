# Copyright (c) 2026 Infineon Technologies AG.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--target-handle=TARGET.cm4")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
board_runner_args(jlink "--device=CYW55513")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
