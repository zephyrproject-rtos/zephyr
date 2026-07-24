# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Flash variant selected by board revision:
#   mx25um51345g - as-shipped octal MX25UM51345G on XSPI0 (default)
#   w25q512nw    - EVK reworked to the on-board W25Q512NW quad SPI NOR on XSPI0
set(BOARD_REVISIONS "mx25um51345g" "w25q512nw")
if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION "mx25um51345g")
else()
  if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
    message(FATAL_ERROR
      "${BOARD_REVISION} is not a valid revision for mimxrt700_evk. "
      "Accepted revisions: ${BOARD_REVISIONS}")
  endif()
endif()
