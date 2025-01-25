# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=kw45b41z83" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
