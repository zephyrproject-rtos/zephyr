if(DEFINED BOARD_REVISION AND NOT BOARD_REVISION STREQUAL "sdram")
  message(FATAL_ERROR
    "Invalid board revision '${BOARD_REVISION}'. "
    "Valid revisions: sdram (HyperRAM replaced by SDRAM W9825G6KH x2)."
  )
endif()
