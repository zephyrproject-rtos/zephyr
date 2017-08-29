zephyr_library()
zephyr_library_sources(pinmux.c)


zephyr_library_include_directories(PRIVATE
  ${PROJECT_SOURCE_DIR}/drivers
  )
