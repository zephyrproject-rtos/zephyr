# Copyright (c) 2025, FocalTech Systems CO.,Ltd
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=FT9001" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
