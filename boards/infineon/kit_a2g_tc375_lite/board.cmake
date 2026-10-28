# SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

# Needs an OpenOCD build with AURIX support
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
