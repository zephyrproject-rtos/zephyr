set(BOARD_REVISIONS "A0" "A1")

# If BOARD_REVISION not set, use the default revision
if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION ${LIST_BOARD_REVISION_DEFAULT})
endif()

if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
  message(FATAL_ERROR "${BOARD_REVISION} is not a valid revision for PocketBeagle 2. Accepted revisions: ${BOARD_REVISIONS}")
endif()

if("${BOARD_QUALIFIERS}" MATCHES "/am6232/*" AND "${BOARD_REVISION}" STREQUAL "A1")
  set(ACTIVE_BOARD_REVISION "A0")
  message(WARNING "A1 only supports AM6254. Using A0 instead.")
endif()

if("${BOARD_QUALIFIERS}" MATCHES "/am6254/*" AND "${BOARD_REVISION}" STREQUAL "A0")
  set(ACTIVE_BOARD_REVISION "A1")
  message(WARNING "A0 only supports AM6254. Using A1 instead.")
endif()
