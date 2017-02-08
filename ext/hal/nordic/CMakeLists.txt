if(CONFIG_HAS_NORDIC_MDK)
  target_include_directories(zephyr PUBLIC mdk)
endif()

if(CONFIG_HAS_NORDIC_HAL)
  target_include_directories(zephyr PUBLIC hal)
endif()

if(CONFIG_HAS_NORDIC_DRIVERS)
  target_include_directories(zephyr PUBLIC drivers)
endif()
