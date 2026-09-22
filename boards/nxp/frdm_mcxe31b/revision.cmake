# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# The revision list and default come from board.yml.
if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION ${LIST_BOARD_REVISION_DEFAULT})
elseif(NOT BOARD_REVISION IN_LIST LIST_BOARD_REVISIONS)
  message(FATAL_ERROR
    "${BOARD_REVISION} is not a valid revision for frdm_mcxe31b. "
    "Accepted revisions: ${LIST_BOARD_REVISIONS}")
endif()
