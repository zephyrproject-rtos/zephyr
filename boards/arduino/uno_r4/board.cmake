# Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
# SPDX-License-Identifier: Apache-2.0

if(NOT ("${BOARD_QUALIFIERS}" MATCHES "minima"
      OR "${BOARD_QUALIFIERS}" MATCHES "wifi"))
  message(FATAL_ERROR "Please specify a board variant for the arduino_uno_r4:\n"
                      "arduino_uno_r4/r7fa4m1ab3cfm/minima or arduino_uno_r4/r7fa4m1ab3cfm/wifi\n")
endif()

board_runner_args(pyocd "--target=r7fa4m1ab")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
