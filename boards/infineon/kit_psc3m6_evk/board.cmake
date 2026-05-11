# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--target-handle=TARGET.cm33")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
