# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})
add_subdirectory(common)

zephyr_sources(
  validate_base_addresses.c
  validate_enabled_instances.c
  )

if(CONFIG_SOC_HAS_TIMING_FUNCTIONS AND NOT CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
  if(CONFIG_TIMING_FUNCTIONS)
    # Use nRF-specific timing calculations only if DWT is not present
    if(NOT CONFIG_CORTEX_M_DWT)
      zephyr_library_sources(timing.c)
    endif()
  endif()
endif()

if(CONFIG_BUILD_WITH_TFM)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DHAL_NORDIC_PATH=${ZEPHYR_HAL_NORDIC_MODULE_DIR}
    )
endif()
