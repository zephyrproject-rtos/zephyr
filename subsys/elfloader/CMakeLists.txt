if(CONFIG_DYNAMIC_MODULE)

  zephyr_library()
  zephyr_library_sources(elfloader.c)

endif()
