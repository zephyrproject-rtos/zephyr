# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(mplab_ipe "--tool" "PKOB4" "--part" "32CZ8110CA90208" "--erase" "--verify")
board_runner_args(jlink "--device=PIC32CZ8110CA90" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/mplab_ipe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
