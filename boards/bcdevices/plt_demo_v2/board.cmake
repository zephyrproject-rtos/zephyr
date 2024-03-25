# SPDX-License-Identifier: Apache-2.0

set(OPENOCD_NRF5_SUBFAMILY "nrf52")
board_runner_args(nrfjprog "--nrf-family=NRF52")
board_runner_args(jlink "--device=nRF52832_xxAA" "--speed=4000")
board_runner_args(pyocd "--target=nrf52832" "--frequency=4000000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-nrf5.board.cmake)
