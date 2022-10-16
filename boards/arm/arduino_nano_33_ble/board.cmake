# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
