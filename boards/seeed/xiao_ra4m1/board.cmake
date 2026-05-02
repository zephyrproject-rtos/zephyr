# Copyright (c) 2025 Pete Johanson
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R7FA4M1AB")
board_runner_args(rfp "--device=RA")

include(${ZEPHYR_BASE}/boards/common/rfp.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
