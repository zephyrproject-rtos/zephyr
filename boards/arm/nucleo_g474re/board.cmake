# SPDX-License-Identifier: Apache-2.0

# use target=stm32g474rbtx instead of stm32g474retx
# to allow board re-flashing (see PR #23230)
board_runner_args(pyocd "--target=stm32g474rbtx")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
