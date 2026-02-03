set(BOARD_REVISIONS "C5" "C7")

if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION ${LIST_BOARD_REVISION_DEFAULT})
endif()

if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
  message(FATAL_ERROR "${BOARD_REVISION} is not a valid revision for BeagleConnect Freedom. Accepted revisions: ${BOARD_REVISIONS}")
endif()

if("${BOARD_QUALIFIERS}" MATCHES "^/cc1352p7$" AND "${BOARD_REVISION}" STREQUAL "C5")
  message(FATAL_ERROR "C5 only supports CC1352P. Use C7 instead.")
endif()

if("${BOARD_QUALIFIERS}" MATCHES "^/cc1352p$" AND "${BOARD_REVISION}" STREQUAL "C7")
  message(FATAL_ERROR "C7 only supports CC1352P7. Use C5 instead.")
endif()
