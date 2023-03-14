# Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=atsam3u4e")
board_runner_args(jlink "--reset-after-load")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
