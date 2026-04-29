# Copyright (c) 2025 emtrion GmbH
# SPDX-License-Identifier: Apache-2.0

set(EMSBC_REVISIONS "R2" "R3")
if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION "R2")
else()
  if(NOT BOARD_REVISION IN_LIST EMSBC_REVISIONS)
    message(FATAL_ERROR "${BOARD_REVISION} is not a valid revision for emsbc_neon_cm7. Accepted revisions: ${EMSBC_REVISIONS}")
  endif()
endif()
