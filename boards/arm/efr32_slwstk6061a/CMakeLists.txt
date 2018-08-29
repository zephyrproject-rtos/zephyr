if(CONFIG_UART_GECKO)
  target_sources(board PRIVATE ${CMAKE_CURRENT_LIST_DIR}/board.c)
  target_include_directories(board PRIVATE ${PROJECT_SOURCE_DIR}/drivers)
endif()
