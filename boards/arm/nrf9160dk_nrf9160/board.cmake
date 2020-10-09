# SPDX-License-Identifier: Apache-2.0

# Set the corresponding TF-M target platform when building for the Non-Secure
# version of the board (Application MCU).
if(CONFIG_BOARD_NRF9160DK_NRF9160NS)
  set(TFM_TARGET_PLATFORM "nordic_nrf/nrf9160dk_nrf9160")
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

board_runner_args(nrfjprog "--nrf-family=NRF91")
board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
