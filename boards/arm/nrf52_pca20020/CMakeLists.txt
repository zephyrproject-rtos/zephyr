target_sources(board PRIVATE ${CMAKE_CURRENT_LIST_DIR}/board.c)
target_include_directories(board PRIVATE ${ZEPHYR_BASE}/drivers)
