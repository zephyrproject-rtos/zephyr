add_library(app_memory STATIC ${CMAKE_CURRENT_LIST_DIR}/app_memdomain.c)
target_link_libraries(app_memory PRIVATE zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE app_memory)
zephyr_set_property(TARGET app_memory MEMORY_SPACE kernel)
