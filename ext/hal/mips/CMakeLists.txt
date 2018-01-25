if(CONFIG_HAS_MIPS_HAL)
  zephyr_include_directories(hal)
  zephyr_include_directories(include)

  add_subdirectory(hal)
endif()
