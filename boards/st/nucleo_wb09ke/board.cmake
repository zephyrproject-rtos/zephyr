# SPDX-License-Identifier: Apache-2.0
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=sw")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
