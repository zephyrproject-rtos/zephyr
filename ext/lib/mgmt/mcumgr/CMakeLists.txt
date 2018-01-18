zephyr_interface_library_named(MCUMGR)

zephyr_library()
add_subdirectory(cborattr)
add_subdirectory(cmd)
add_subdirectory(mgmt)
add_subdirectory(smp)
add_subdirectory(util)

zephyr_library_link_libraries(MCUMGR)

target_link_libraries(MCUMGR INTERFACE
  zephyr_interface
  TINYCBOR
  )
