# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NUMAKER_PFM_M467)
  board_runner_args(pyocd "--target=m467hjhae")
endif()

board_runner_args(nulink "-f")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nulink.board.cmake)

if(CONFIG_BOARD_NUMAKER_PFM_M467)
  include(${ZEPHYR_BASE}/boards/common/canopen.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
endif()
