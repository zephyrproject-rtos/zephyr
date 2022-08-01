# SPDX-License-Identifier: Apache-2.0

# board_runner_args(stm32cubeprogrammer "--port=swd" "--reset=hw")
board_runner_args(dfu-util "--pid=18d1:1234" "--alt=1" "--dfuse")

# include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)

