set(NRF5340DK_NRF5340_CPUAPP_IMAGES)
set(NRF5340DK_NRF5340_CPUAPP_NS_IMAGES)
set(NRF5340DK_NRF5340_CPUNET_IMAGES)
set(UNKNOWN_IMAGES)

# Order images into which boards they target, this should be done at a SoC
# level, but optionally at a board level too
foreach(image ${IMAGES})
  load_cache(IMAGE ${image} BINARY_DIR "${CMAKE_BINARY_DIR}/${image}")

  set(image_board)
  sysbuild_get(image_board IMAGE ${image} VAR CONFIG_BOARD KCONFIG)

  # These should not use board names and should be extracted from device tree
  if("${image_board}" STREQUAL "nrf5340dk_nrf5340_cpuapp")
    list(APPEND NRF5340DK_NRF5340_CPUAPP_IMAGES ${image})
  elseif("${image_board}" STREQUAL "nrf5340dk_nrf5340_cpuapp_ns")
    list(APPEND NRF5340DK_NRF5340_CPUAPP_NS_IMAGES ${image})
  elseif("${image_board}" STREQUAL "nrf5340dk_nrf5340_cpunet")
    list(APPEND NRF5340DK_NRF5340_CPUNET_IMAGES ${image})
  else()
    list(APPEND UNKNOWN_IMAGES ${image})
  endif()
endforeach()

# Re-order images for network core, application core (secure), application
# core (non-secure) then other targets, possibly update a FLASH_DEPENDS
# variable for each image here
set(IMAGES)

foreach(image ${NRF5340DK_NRF5340_CPUNET_IMAGES})
  list(APPEND IMAGES ${image})
endforeach()

foreach(image ${NRF5340DK_NRF5340_CPUAPP_IMAGES})
  list(APPEND IMAGES ${image})
endforeach()

foreach(image ${NRF5340DK_NRF5340_CPUAPP_NS_IMAGES})
  list(APPEND IMAGES ${image})
endforeach()

foreach(image ${UNKNOWN_IMAGES})
  list(APPEND IMAGES ${image})
endforeach()
