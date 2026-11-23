#
# Copyright (c) 2025 Space Cubics Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

if("${BOARD_QUALIFIERS}" STREQUAL "versal_rpu")
  include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)
elseif("${BOARD_QUALIFIERS}" STREQUAL "miv")
  board_runner_args(openocd "--file-type=elf")
  board_runner_args(openocd "--cmd-reset-halt=halt")

  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
else()
  message(FATAL_ERROR "Unsupported SC-OBC V1 board qualifiers: ${BOARD_QUALIFIERS}")
endif()
