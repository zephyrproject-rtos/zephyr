# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

# OpenOCD cfg
board_runner_args(openocd "--config=${ZEPHYR_BASE}/boards/infineon/cy8ckit_041s_max/support/openocd.cfg")

# Include standard OpenOCD runner helpers
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
