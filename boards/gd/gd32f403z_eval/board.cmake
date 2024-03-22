# Copyright (c) 2021, ATL Electronics
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(gd32isp "--device=GD32F403ZET6")
include(${ZEPHYR_BASE}/boards/common/gd32isp.board.cmake)
