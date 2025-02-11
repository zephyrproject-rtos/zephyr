# SPDX-License-Identifier: Apache-2.0

board_runner_args(nrfjprog "--softreset")
board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
