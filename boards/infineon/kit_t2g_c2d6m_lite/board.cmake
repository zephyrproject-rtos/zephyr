# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

# Connect to CM0P core, the CYT4DN target configuration names the chip traveo2.
board_runner_args(openocd "--target-handle=traveo2.cpu.cm0")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
