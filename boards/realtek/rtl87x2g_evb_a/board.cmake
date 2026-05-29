# Copyright (c) 2026, Realtek Semiconductor Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=RTL87X2G" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/mpcli.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
