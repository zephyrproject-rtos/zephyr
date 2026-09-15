#
# Copyright (c) 2026 Mario Paja
# SPDX-License-Identifier: Apache-2.0
#

set(BOARD_REVISIONS "C2" "D1")

# No revision at all builds the plain board, with no audio codec wired in.
if(DEFINED BOARD_REVISION AND NOT "${BOARD_REVISION}" STREQUAL "")
  if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
    message(FATAL_ERROR
      "${BOARD_REVISION} is not a valid revision for stm32h573i_dk. "
      "Accepted revisions: ${BOARD_REVISIONS}")
  endif()
endif()
