# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(bhi2xy)

zephyr_library_sources(bhi2xy.c bhi2xy_parsers.c bhi2xy_unit_conversions.c)
zephyr_library_sources_ifdef(CONFIG_BHI2XY_BUS_SPI bhi2xy_spi.c)
zephyr_library_sources_ifdef(CONFIG_BHI2XY_BUS_I2C bhi2xy_i2c.c)

if(CONFIG_BHI2XY_BHI260AP_SAMPLE_FIRMWARE_FILE)
# Check if preprovided firmware is used
  set(BHI2XY_FW_PATH "${ZEPHYR_BOSCH_APIS_MODULE_DIR}/zephyr/blobs")
  set(BHI260AP_FW_FILE_NAME "BHI260AP.fw")
  set(BHI260AP_FW_PATH "${BHI2XY_FW_PATH}/${BHI260AP_FW_FILE_NAME}")
  set(FIRMWARE_FILE_NAME ${BHI260AP_FW_FILE_NAME})
  set(FIRMWARE_PATH ${BHI260AP_FW_PATH})
else()
  # Embed sensor firmware file
  # First check if the firmware file exists in the project root.
  # Then, copy it to the build directory for the driver and rename it to a fixed name.
  # Finally, embed it into the driver as binary blob.
  set(FIRMWARE_FILE_NAME "${CONFIG_BHI2XY_FIRMWARE_FILE}")
  set(PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
  set(FIRMWARE_PROJECT_PATH "${PROJECT_SOURCE_DIR}/${FIRMWARE_FILE_NAME}")
  set(FIRMWARE_PATH ${FIRMWARE_PROJECT_PATH})
endif()

if(EXISTS "${FIRMWARE_PATH}")
    message(STATUS "BHI2xy: Using firmware file: ${FIRMWARE_PATH}")
    file(COPY "${FIRMWARE_PATH}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    set(FIRMWARE_FOUND_PATH "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILE_NAME}")
else()
  message(FATAL_ERROR "BHI2xy: Firmware file '${FIRMWARE_FILE_NAME}' not found: ${FIRMWARE_PATH}!")
endif()

set(FIRMWARE_FIXED_NAME "bhi2xy.fw")
file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILE_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FIXED_NAME}")

# Embed the firmware file into the application
include(embed_file.cmake)
add_embedded_resources(bhi2xy "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FIXED_NAME}")
