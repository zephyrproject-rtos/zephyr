# Copyright (c) 2019 Interay Solutions B.V.
# Copyright (c) 2019 Oane Kingma
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFM32GG11B820F2048")
board_runner_args(openocd)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
