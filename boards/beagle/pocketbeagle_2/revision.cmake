set(BOARD_REVISIONS "A0" "A1")

# If BOARD_REVISION not set, use the default revision
if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION ${LIST_BOARD_REVISION_DEFAULT})
endif()

if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
  message(FATAL_ERROR "${BOARD_REVISION} is not a valid revision for PocketBeagle 2. Accepted revisions: ${BOARD_REVISIONS}")
endif()

if("${BOARD_QUALIFIERS}" MATCHES "/am6232/*" AND "${BOARD_REVISION}" STREQUAL "A1")
  message(FATAL_ERROR "${BOARD_REVISION} only supports AM6254")
endif()

if("${BOARD_QUALIFIERS}" MATCHES "/am6254/*" AND "${BOARD_REVISION}" STREQUAL "A0")
  message(FATAL_ERROR "${BOARD_REVISION} only supports AM6254")
endif()
