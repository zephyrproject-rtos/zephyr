set(BOARD_REVISIONS "A0")

# If BOARD_REVISION not set, use the default revision
if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION ${LIST_BOARD_REVISION_DEFAULT})
endif()

if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
  message(FATAL_ERROR "${BOARD_REVISION} is not a valid revision for PocketBeagle 2. Accepted revisions: ${BOARD_REVISIONS}")
endif()
