# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--erase" "--port=swd" "--reset-mode=hw")
board_runner_args(pyocd "--target=stm32h573iikx")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
# FIXME: openocd runner not yet available.
