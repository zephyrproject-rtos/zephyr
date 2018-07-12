zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
target_sources(soc_xtensa PRIVATE ${CMAKE_CURRENT_LIST_DIR}/soc.c)
