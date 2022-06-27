# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=nrf52833")
board_runner_args(nrfjprog "--nrf-family=NRF52")
board_runner_args(jlink "--device=nRF52833_xxAA" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
