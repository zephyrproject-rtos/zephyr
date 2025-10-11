# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=BGM220PC22HNA" "--reset-after-load")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=BGM220PC22HNA")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
