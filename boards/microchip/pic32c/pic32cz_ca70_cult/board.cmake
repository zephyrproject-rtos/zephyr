# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(mplab_ipe "--tool" "PKOB4" "--part" "32CZ2051CA70144" "--verify")
board_runner_args(jlink "--device=PIC32CZ2051CA70144" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/mplab_ipe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
