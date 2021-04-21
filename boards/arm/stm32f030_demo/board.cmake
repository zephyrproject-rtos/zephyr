#
# Copyright (c) 2019 Antony Pavlov <antonynpavlov@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(stm32flash "--start-addr=0x08000000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32flash.board.cmake)
