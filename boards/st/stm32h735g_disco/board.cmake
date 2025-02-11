# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021 SILA Embedded Solutions GmbH

board_runner_args(jlink "--device=STM32H735IG" "--speed=4000")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
