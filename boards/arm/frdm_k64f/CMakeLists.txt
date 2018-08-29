if(CONFIG_PINMUX_MCUX)
  target_sources(board PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pinmux.c)
target_include_directories(board PRIVATE ${ZEPHYR_BASE}/drivers)
endif()
