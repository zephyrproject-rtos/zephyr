# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=nRF52840_xxAA" "--speed=4000")
board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")
if(CONFIG_BUILD_OUTPUT_UF2)
  include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
endif()
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
