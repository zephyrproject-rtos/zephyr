set(BOARD_REVISIONS "n16r8" "a128r8")

if(NOT DEFINED BOARD_REVISION)
  set(BOARD_REVISION "n16r8")
else()
  if(NOT BOARD_REVISION IN_LIST BOARD_REVISIONS)
    message(FATAL_ERROR "${BOARD_REVISION} is not a valid revision for sf32lb52_devkit_lcd. Accepted revisions: ${BOARD_REVISIONS}")
  endif()
endif()
