# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
# FIXME: openocd runner not yet available.
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
