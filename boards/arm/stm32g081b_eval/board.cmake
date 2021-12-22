# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset=hw")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)

