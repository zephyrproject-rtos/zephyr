# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32flash "--baud-rate=115200" "--start-addr=0x08000000")

include(${ZEPHYR_BASE}/boards/common/stm32flash.board.cmake)
