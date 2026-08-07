# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=pic32cm6408pl10048" "--frequency=100000")
board_runner_args(mplab_ipe "--tool" "nEDBG" "--part" "32CM6408PL10048" "--erase" "--verify")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/mplab_ipe.board.cmake)
