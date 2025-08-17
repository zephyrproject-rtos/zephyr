# Copyright (c) 2025 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=PIC32CM5164JH" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
