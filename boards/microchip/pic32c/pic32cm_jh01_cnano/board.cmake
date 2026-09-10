# Copyright (c) 2025-2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=pic32cm5164jh01048" "--frequency=400000")
board_runner_args(mplab_ipe "--tool" "nEDBG" "--part" "32CM5164JH01048" "--erase" "--verify")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/mplab_ipe.board.cmake)
