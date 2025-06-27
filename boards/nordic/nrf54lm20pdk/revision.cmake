# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set(REVISIONS "0.0.0" "0.2.0" "0.2.0.csp")
if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION "0.2.0.csp")
else()
  if(NOT BOARD_REVISION IN_LIST REVISIONS)
    message(FATAL_ERROR "${BOARD_REVISION} is not a valid nRF54LM20 PDK board revision. Accepted revisions: ${REVISIONS}")
  endif()
endif()
