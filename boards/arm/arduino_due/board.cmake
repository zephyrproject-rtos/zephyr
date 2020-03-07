# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)

board_runner_args(jlink "--device=atsam3x8e" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
