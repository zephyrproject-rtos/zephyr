zephyr_sources_ifdef(
  CONFIG_OPENOCD_SUPPORT
  openocd.c
  )

add_subdirectory(tracing)
