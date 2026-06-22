# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=PIC32CM5112GC" "--speed=auto")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
