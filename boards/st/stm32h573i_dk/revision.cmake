#
# Copyright (c) 2026 Mario Paja
# SPDX-License-Identifier: Apache-2.0
#

set(BOARD_REVISIONS "C2" "D1")

# No revision specified: fall back to the default
if(NOT DEFINED BOARD_REVISION OR "${BOARD_REVISION}" STREQUAL "")
  set(BOARD_REVISION ${LIST_BOARD_REVISION_DEFAULT})
endif()

if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
  message(FATAL_ERROR
    "${BOARD_REVISION} is not a valid revision for stm32h573i_dk. "
    "Accepted revisions: ${BOARD_REVISIONS}")
endif()
