# SPDX-License-Identifier: Apache-2.0

# AP1302 driver
zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/ap1302.c)

# Optional: embed an AP1302 firmware blob into the final image.
#
# Usage:
#   - Enable CONFIG_VIDEO_AP1302_BUILTIN_FIRMWARE=y
#   - Provide the blob path at build time:
#       west build ... -- -DAP1302_FIRMWARE_FILE=/abs/path/to/ap1302_fw.bin
#
# Implementation:
#   The blob is converted to a generated .c file at build time, which avoids
#   depending on objcopy or target-specific object formats.
if(CONFIG_VIDEO_AP1302_BUILTIN_FIRMWARE)
  set(AP1302_FIRMWARE_FILE "" CACHE FILEPATH "Path to the AP1302 firmware binary blob")

  if(NOT AP1302_FIRMWARE_FILE)
    message(FATAL_ERROR
      "CONFIG_VIDEO_AP1302_BUILTIN_FIRMWARE is enabled but AP1302_FIRMWARE_FILE is not set.\n"
      "Provide it at build time, e.g.:\n"
      "  west build ... -- -DAP1302_FIRMWARE_FILE=<path-to-ap1302-fw.bin>\n")
  endif()

  if(NOT EXISTS ${AP1302_FIRMWARE_FILE})
    message(FATAL_ERROR
      "AP1302_FIRMWARE_FILE does not exist: ${AP1302_FIRMWARE_FILE}")
  endif()

  include(${ZEPHYR_BASE}/cmake/modules/python.cmake)

  set(AP1302_FW_C "${CMAKE_CURRENT_BINARY_DIR}/ap1302_fw.c")

  add_custom_command(
    OUTPUT ${AP1302_FW_C}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/bin2c.py
            --input ${AP1302_FIRMWARE_FILE}
            --output ${AP1302_FW_C}
            --symbol ap1302_fw_blob
    DEPENDS ${AP1302_FIRMWARE_FILE} ${CMAKE_CURRENT_LIST_DIR}/bin2c.py
    VERBATIM
  )

  # Ensure generation happens before compiling the library.
  add_custom_target(ap1302_fw_c ALL DEPENDS ${AP1302_FW_C})
  add_dependencies(drivers__video ap1302_fw_c)

  zephyr_library_sources(${AP1302_FW_C})
endif()
