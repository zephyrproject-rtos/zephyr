zephyr_library()
zephyr_library_sources(${CMAKE_CURRENT_SOURCE_DIR}/pinmux.c)
target_link_libraries(boards__x86__arduino_101 zephyr drivers__pinmux)
target_include_directories(boards__x86__arduino_101 PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/drivers
  ${PROJECT_SOURCE_DIR}/drivers/pinmux
  )
