# Copyright (c) 2026 Benjamin Schilling <benjamin.schilling33@gmail.com>

set(BOARD_REVISIONS "rs485" "can")
if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION "rs485")
else()
  if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
    message(FATAL_ERROR "${BOARD_REVISION} is not a valid revision for esp32s3_eth_8di_8ro. Accepted revisions: ${BOARD_REVISIONS}")
  endif()
endif()
