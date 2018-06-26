add_library(subsys__bluetooth INTERFACE)

target_include_directories(subsys__bluetooth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_BT_HCI host)
add_subdirectory_ifdef(CONFIG_BT_SHELL shell)

if(CONFIG_BT_CTLR)
  if(CONFIG_BT_LL_SW)
    add_subdirectory(controller)
  endif()
endif()

target_link_libraries(subsys__bluetooth INTERFACE zephyr_interface)
