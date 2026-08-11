# Copyright (c) 2025 u-blox AG
# SPDX-License-Identifier: Apache-2.0

set(BOARD_REVISIONS "fidelix" "fidelix_16mb" "macronix" "macronix_16mb")

if(NOT BOARD_REVISION)
  set(BOARD_REVISION "fidelix")
endif()

# Validate revision
if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
  message(FATAL_ERROR
    "Invalid BOARD_REVISION: ${BOARD_REVISION}\n"
    "Must be one of: ${BOARD_REVISIONS}"
  )
endif()
