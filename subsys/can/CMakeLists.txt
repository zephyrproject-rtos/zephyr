if(CONFIG_CAN_STACKS)
  zephyr_interface_library_named(CAN_STACKS)

  zephyr_library()
  zephyr_library_sources(common.c)
  zephyr_library_link_libraries(CAN_STACKS)


  add_subdirectory_ifdef(CONFIG_CANOPEN    ./canopen)
  add_subdirectory_ifdef(CONFIG_DEVICENET  ./devicenet)
  add_subdirectory_ifdef(CONFIG_J1939      ./j1939)
endif()
