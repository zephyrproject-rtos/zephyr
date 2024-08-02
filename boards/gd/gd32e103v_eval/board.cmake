# Copyright (c) 2021, YuLong Yao <feilongphone@gmail.com>
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(gd32isp "--device=GD32E103VBT6")
include(${ZEPHYR_BASE}/boards/common/gd32isp.board.cmake)
