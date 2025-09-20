# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(bhi2xy)

zephyr_library_sources(bhi2xy.c bhi2xy_parsers.c bhi2xy_unit_conversions.c)
zephyr_library_sources_ifdef(CONFIG_BHI2XY_BUS_SPI bhi2xy_spi.c)
zephyr_library_sources_ifdef(CONFIG_BHI2XY_BUS_I2C bhi2xy_i2c.c)

# Sources from Bosch API
zephyr_library_include_directories(${ZEPHYR_BASE}/../modules/lib/bhi2xy)
zephyr_library_sources(${ZEPHYR_BASE}/../modules/lib/bhi2xy/bhy2.c)
zephyr_library_sources(${ZEPHYR_BASE}/../modules/lib/bhi2xy/bhy2_bsec.c)
zephyr_library_sources(${ZEPHYR_BASE}/../modules/lib/bhi2xy/bhy2_hif.c)
zephyr_library_sources(${ZEPHYR_BASE}/../modules/lib/bhi2xy/bhy2_parse.c)

# Embed sensor firmware file
# First, check if the firmware file exists in the project root.
# Then, copy it to the build directory for the driver and rename it to a fixed name.
# Finally, embed it into the driver as binary blob.
set(FIRMWARE_FILE_NAME "${CONFIG_BHI2XY_FIRMWARE_FILE}")
set(PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
set(FIRMWARE_PROJECT_PATH "${PROJECT_SOURCE_DIR}/${FIRMWARE_FILE_NAME}")

if(EXISTS "${FIRMWARE_PROJECT_PATH}")
    message(STATUS "Found firmware file in project dir: ${FIRMWARE_PROJECT_PATH}")
    file(COPY "${FIRMWARE_PROJECT_PATH}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    set(FIRMWARE_FOUND_PATH "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILE_NAME}")
else()
    message(FATAL_ERROR "Firmware file '${FIRMWARE_FILE_NAME}' not found in project directory!")
endif()

set(FIRMWARE_FIXED_NAME "bhi2xy.fw")
file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILE_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FIXED_NAME}")

# Embed the firmware file into the application
include(embed_file.cmake)
add_embedded_resources(bhi2xy "${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FIXED_NAME}")
