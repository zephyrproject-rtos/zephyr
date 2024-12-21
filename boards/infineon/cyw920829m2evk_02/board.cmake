# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
board_runner_args(jlink "--device=CYW20829_tm")
include (${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
