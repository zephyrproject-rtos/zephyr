# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=stm32f407zg" "--frequency=4000000")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
