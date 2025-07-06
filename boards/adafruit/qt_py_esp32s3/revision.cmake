if(DEFINED BOARD_REVISION AND NOT BOARD_REVISION STREQUAL "psram")
  message(FATAL_ERROR "Invalid board revision, ${BOARD_REVISION}, valid revisions are: <none> (for non-PSRAM version), psram")
endif()
