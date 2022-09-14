# Copyright (c) 2022 Byte-Lab d.o.o. <dev@byte-lab.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32H7B3LI" "--speed=4000")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
