if(CONFIG_PINMUX_SAM0)
  target_sources(board PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pinmux.c)
endif()
