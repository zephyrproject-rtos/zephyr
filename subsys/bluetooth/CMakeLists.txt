add_library(subsys__bluetooth INTERFACE)
zephyr_append_cmake_library(subsys__bluetooth)

target_include_directories(subsys__bluetooth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_BT_HCI host)
add_subdirectory_ifdef(CONFIG_BT_CTLR controller)

# TODO: Add shell

target_link_libraries(subsys__bluetooth INTERFACE zephyr drivers)
