if(CONFIG_HAS_NORDIC_DRIVERS OR CONFIG_HAS_NRFX)
  zephyr_library()
endif()

if(CONFIG_HAS_NORDIC_DRIVERS)
  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers drivers)
endif()

if(CONFIG_HAS_NRFX)
  add_subdirectory(nrfx)
endif()
