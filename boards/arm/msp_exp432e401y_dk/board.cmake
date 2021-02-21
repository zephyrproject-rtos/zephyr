# SPDX-License-Identifier: Apache-2.0

board_runner_args(xds110 "--device=MSP432E401Y" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
