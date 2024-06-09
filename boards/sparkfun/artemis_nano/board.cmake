# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=cortex_m" "--frequency=4000000" "--probe=/dev/ttyUSB0")

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/trace32.board.cmake)
