# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(mplab_ipe "--tool" "PKOB4" "--part" "32CK2051GC01144" "--erase" "--verify")
board_runner_args(jlink "--device=PIC32CK2051GC" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/mplab_ipe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
