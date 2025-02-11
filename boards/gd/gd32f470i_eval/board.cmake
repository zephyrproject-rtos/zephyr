# Copyright (c) 2022, Rtone.
# SPDX-License-Identifier: Apache-2.0

# GD32F470xx series is not yet supported by SEGGER J-Link
board_runner_args(jlink "--device=GD32F450IK" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
