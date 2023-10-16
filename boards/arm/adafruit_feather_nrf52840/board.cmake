# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_ADAFRUIT_FEATHER_NRF52840_SENSE)
  set(ADAFRUIT_FEATHER_NRF52840_ID "nRF52840-Feather-Sense")
else()
  set(ADAFRUIT_FEATHER_NRF52840_ID "nRF52840-Feather-revD")
endif()

board_runner_args(uf2 "--board-id=${ADAFRUIT_FEATHER_NRF52840_ID}")
board_runner_args(jlink "--device=nRF52840_xxAA" "--speed=4000")
board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
