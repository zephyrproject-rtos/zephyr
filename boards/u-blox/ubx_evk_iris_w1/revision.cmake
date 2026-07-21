# Copyright (c) 2025 u-blox AG
# SPDX-License-Identifier: Apache-2.0

set(BOARD_REVISIONS "macronix" "fidelix")

if(NOT BOARD_REVISION)
  set(BOARD_REVISION "fidelix")
endif()

# Validate revision
if(NOT BOARD_REVISION STREQUAL "macronix" AND NOT BOARD_REVISION STREQUAL "fidelix")
  message(FATAL_ERROR
    "Invalid BOARD_REVISION: ${BOARD_REVISION}\n"
    "Must be one of: macronix, fidelix"
  )
endif()
