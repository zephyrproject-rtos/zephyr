# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R7FA8E1AF")
board_runner_args(pyocd "--target=R7FA8E1AF")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
