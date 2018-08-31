add_library(json STATIC ${CMAKE_CURRENT_LIST_DIR}/json.c)
target_link_libraries(json PRIVATE zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE json)
