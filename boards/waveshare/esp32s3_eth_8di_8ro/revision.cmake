if(DEFINED BOARD_REVISION AND NOT BOARD_REVISION STREQUAL "can")
  message(FATAL_ERROR "Invalid board revision, ${BOARD_REVISION}, valid revisions are: <none> (for RS485 version), can")
endif()
