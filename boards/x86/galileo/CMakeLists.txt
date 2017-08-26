zephyr_library()
zephyr_library_sources(pinmux.c)
target_include_directories(boards__x86__galileo PRIVATE
  ${PROJECT_SOURCE_DIR}/drivers
  )
