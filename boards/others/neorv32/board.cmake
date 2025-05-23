# Copyright (c) 2021,2025 Henrik Brix Andersen <henrik@brixandersen.dk>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--use-elf" "--cmd-reset-halt" "halt")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

if("${BOARD_QUALIFIERS}" STREQUAL "/neorv32")
  message(FATAL_ERROR "Please specify a variant for the neorv32 board "
                      "(e.g. neorv32/neorv32/minimalboot or neorv32/neorv32/up5kdemo)")
endif()
