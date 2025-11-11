#SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(teensy)

if(CONFIG_BOARD_TEENSY40)
  board_runner_args(teensy "--mcu=TEENSY40")
elseif(CONFIG_BOARD_TEENSYMM)
  board_runner_args(teensy "--mcu=TEENSY_MICROMOD")
else()
  board_runner_args(teensy "--mcu=TEENSY41")
endif()

include(${ZEPHYR_BASE}/boards/common/teensy.board.cmake)
