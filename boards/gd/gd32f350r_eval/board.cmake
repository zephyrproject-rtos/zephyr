# Copyright (c) 2021 BrainCo Inc.
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(jlink "--device=GD32F350G6" "--iface=SWD" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
