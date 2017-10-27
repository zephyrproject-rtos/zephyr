if(CONFIG_HAS_NORDIC_MDK)
  zephyr_include_directories(mdk)
endif()

if(CONFIG_HAS_NORDIC_HAL)
  zephyr_include_directories(hal)
endif()

if(CONFIG_HAS_NORDIC_DRIVERS)
  zephyr_include_directories(drivers)

  add_subdirectory(drivers)
endif()
