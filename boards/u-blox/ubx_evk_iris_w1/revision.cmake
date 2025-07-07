# Copyright (c) 2025 u-blox AG
# SPDX-License-Identifier: Apache-2.0

if(NOT BOARD_REVISION)
  set(BOARD_REVISION fidelex CACHE STRING "Board revision")
endif()

# Validate revision
if(NOT BOARD_REVISION STREQUAL "macronix" AND NOT BOARD_REVISION STREQUAL "fidelex")
  message(FATAL_ERROR
    "Invalid BOARD_REVISION: ${BOARD_REVISION}\n"
    "Must be one of: macronix, fidelex"
  )
endif()
