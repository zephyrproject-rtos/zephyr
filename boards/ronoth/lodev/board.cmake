# Ronoth LoDev board configuration
# Copyright (c) 2020/2021 Dean Weiten <dmw@weiten.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32L073RZ" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
