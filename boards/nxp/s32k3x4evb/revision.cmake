# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(S32K3X4EVB_REVISIONS "A" "B" "B1" "B2")

if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION "B2")
else()
  if(NOT BOARD_REVISION IN_LIST S32K3X4EVB_REVISIONS)
    message(FATAL_ERROR "${BOARD_REVISION} is not a valid revision for s32k3x4evb_t172. Accepted revisions: ${S32K3X4EVB_REVISIONS}")
  endif()
endif()
