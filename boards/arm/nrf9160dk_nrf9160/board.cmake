# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF9160DK_NRF9160NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

board_runner_args(jlink "--device=nRF9160_xxAA" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
