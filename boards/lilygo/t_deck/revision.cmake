# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(BOARD_REVISIONS "standard" "plus")

if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION "standard")
elseif(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
  message(FATAL_ERROR "Invalid board revision, ${BOARD_REVISION}, valid revisions are: standard, plus")
endif()
