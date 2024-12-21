# Copyright (c) 2022 Atmosic
#
# SPDX-License-Identifier: Apache-2.0

set(ATM33_VARIANT "normal" "mcuboot")
if(DEFINED BOARD_REVISION)
  if (NOT BOARD_REVISION IN_LIST ATM33_VARIANT)
    message(FATAL_ERROR "${BOARD_REVISION} is not a valid variant for ATMEVK33xx. Accepted revisions: ${ATM33_VARIANT}")
  endif()
endif()
