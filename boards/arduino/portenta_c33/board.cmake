# Copyright (c) 2025 Arduino SA
# SPDX-License-Identifier: Apache-2.0


# FIXME: Arduino dfu-util provides -Q to reset the board after flashing
board_runner_args(dfu-util "--pid=2341:0368" "--alt=0")
board_runner_args(jlink "--device=R7FA6M5BH")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
