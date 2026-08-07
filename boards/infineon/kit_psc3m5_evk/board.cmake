# Copyright (c) 2025 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--target-handle=TARGET.cm33")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
