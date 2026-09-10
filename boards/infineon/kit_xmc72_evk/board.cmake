# Copyright (c) 2025 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

# Connect to CM0P core.
board_runner_args(openocd "--target-handle=cat1c.cpu.cm0")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
