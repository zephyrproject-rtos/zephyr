# SPDX-License-Identifier: Apache-2.0

set(TFM_TARGET_PLATFORM "nrf9160dk_nrf9160")
set(TFM_PUBLIC_KEY_FORMAT "full")

board_runner_args(nrfjprog "--nrf-family=NRF91")
board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
