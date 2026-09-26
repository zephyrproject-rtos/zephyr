#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zms_provisioning)

target_sources(app PRIVATE src/main.c)

# This sample provisions data by merging into a flashable zephyr.hex and
# configuring west flash to program the result (see the hex_file override
# below), so it only makes sense on boards that produce a hex output.
# native_sim, for example, builds a host executable instead and has no such
# output or flashing step, so fail early with a clear reason rather than
# hitting a late, cryptic "zephyr.hex missing and no known rule to make it"
# ninja error. Supported boards are listed in this sample's tests.yaml.
if(NOT CONFIG_BUILD_OUTPUT_HEX)
  message(FATAL_ERROR
    "zms_provisioning requires a board with hex output support "
    "(CONFIG_BUILD_OUTPUT_HEX); '${BOARD}' does not produce one. "
    "See this sample's tests.yaml for supported boards."
  )
endif()

# Extract configuration from device tree
# Storage partition node (labeled as storage_partition)
dt_nodelabel(storage_partition_node NODELABEL "storage_partition")

if(DEFINED storage_partition_node)
  # Get storage partition offset and size
  dt_reg_addr(storage_reg_addr PATH ${storage_partition_node})
  dt_reg_size(storage_size PATH ${storage_partition_node})

  # Get flash device from chosen node
  dt_chosen(flash_node PROPERTY "zephyr,flash")

  if(NOT DEFINED flash_node)
    # Fallback: try to get flash device by label
    dt_nodelabel(flash_node NODELABEL "mram1x")
  endif()

  if(DEFINED flash_node)
    # Get NVM base address from flash device. This is only needed here, to
    # resolve the partition's absolute address below; the provisioning
    # script itself only ever deals with the storage area.
    dt_reg_addr(nvm_address PATH ${flash_node})

    # Get write block size from flash device
    dt_prop(write_block_size PATH ${flash_node} PROPERTY "write-block-size")
  else()
    message(WARNING "Flash device node not found. Using defaults for NVM settings.")
    set(nvm_address "0x0")
    set(write_block_size "16")
  endif()

  # A "zephyr,mapped-partition" node's reg is already the absolute address
  # (see DT_MAPPED_PARTITION_ADDR() in
  # include/zephyr/devicetree/mapped-partition.h). A plain fixed-partitions
  # child's reg is only an offset into its parent flash device and needs
  # that device's base address added (see DT_FIXED_PARTITION_ADDR() in
  # include/zephyr/devicetree/fixed-partitions.h). This sample targets
  # arbitrary boards, so both partition styles are handled here instead of
  # assuming one of them.
  dt_prop(storage_partition_compatible PATH ${storage_partition_node} PROPERTY "compatible")

  if(DEFINED storage_partition_compatible AND
     "zephyr,mapped-partition" IN_LIST storage_partition_compatible)
    set(storage_base "${storage_reg_addr}")
  else()
    math(EXPR storage_base "${storage_reg_addr} + ${nvm_address}" OUTPUT_FORMAT HEXADECIMAL)
  endif()

  message(STATUS "Device tree configuration found:")
  message(STATUS "  Storage partition: ${storage_partition_node}")
  if(DEFINED flash_node)
    message(STATUS "  Flash device: ${flash_node}")
  endif()
else()
  message(WARNING "Device tree node 'storage_partition' not found. Using default values.")
  set(storage_base "0x0")
  set(storage_size "0x1000")
  set(write_block_size "16")
endif()

# Other configuration variables
set(ZMS_SECTOR_SIZE "0x1000" CACHE STRING "Sector size in bytes in hex format")
set(ZMS_PROVISION_DATA_FILE "example_data.txt" CACHE STRING "Optional input file with ID-data pairs")
set(ZMS_PROVISION_DATA "" CACHE STRING "Optional ID-data pairs (semicolon-separated)")
set(ZMS_OUTPUT_NAME "provisioned_raw.hex" CACHE STRING
  "Output file name for the standalone provisioning-only image")

# Path to the provisioning script
set(PROVISION_SCRIPT ${ZEPHYR_BASE}/scripts/build/gen_zms_provision_data.py)

# Output directory and files
set(PROVISION_OUTPUT_DIR ${CMAKE_BINARY_DIR}/zephyr)
set(APP_HEX_FILE ${PROVISION_OUTPUT_DIR}/zephyr.hex)
set(PROVISION_OUTPUT_FILE ${PROVISION_OUTPUT_DIR}/${ZMS_OUTPUT_NAME})

# Build the command line arguments for the provisioning script.
set(PROVISION_ARGS
  -f ${storage_base}
  -s ${storage_size}
  -w ${write_block_size}
  -c ${ZMS_SECTOR_SIZE}
)

# Add data file if specified
if(ZMS_PROVISION_DATA_FILE)
  list(APPEND PROVISION_ARGS -i ${ZMS_PROVISION_DATA_FILE})
endif()

# Add individual data pairs if specified
if(ZMS_PROVISION_DATA)
  string(REPLACE ";" " -d " DATA_ARGS "${ZMS_PROVISION_DATA}")
  list(APPEND PROVISION_ARGS -d ${DATA_ARGS})
endif()

# Validate that at least one data source is provided
if(NOT ZMS_PROVISION_DATA_FILE AND NOT ZMS_PROVISION_DATA)
  message(WARNING "No provisioning data specified. Set ZMS_PROVISION_DATA_FILE or ZMS_PROVISION_DATA.")
endif()

set(PROVISION_DEPENDS ${PROVISION_SCRIPT})

# Create custom command to generate the standalone provisioning-only image
add_custom_command(
  OUTPUT ${PROVISION_OUTPUT_FILE}
  COMMAND ${CMAKE_COMMAND} -E remove -f ${PROVISION_OUTPUT_FILE}
  COMMAND ${PYTHON_EXECUTABLE} ${PROVISION_SCRIPT} ${PROVISION_ARGS} -o ${PROVISION_OUTPUT_FILE}
  DEPENDS ${PROVISION_DEPENDS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating ZMS provisioning data: ${ZMS_OUTPUT_NAME}"
  VERBATIM
)

# Create custom target for the provisioning
add_custom_target(provision_raw_data ALL
  DEPENDS ${PROVISION_OUTPUT_FILE}
)

# Merge the standalone provisioning image with the application hex, so a
# single west flash call programs both the application and the provisioning
# data.
set(PROVISION_COMBINED_FILE ${PROVISION_OUTPUT_DIR}/zephyr_with_provision.hex)
set(MERGEHEX_SCRIPT ${ZEPHYR_BASE}/scripts/build/mergehex.py)

add_custom_command(
  OUTPUT ${PROVISION_COMBINED_FILE}
  COMMAND ${CMAKE_COMMAND} -E remove -f ${PROVISION_COMBINED_FILE}
  COMMAND ${PYTHON_EXECUTABLE} ${MERGEHEX_SCRIPT}
          ${PROVISION_OUTPUT_FILE} ${APP_HEX_FILE}
          -o ${PROVISION_COMBINED_FILE}
  DEPENDS ${PROVISION_OUTPUT_FILE} ${APP_HEX_FILE}
  COMMENT "Merging ZMS provisioning data with application hex: zephyr_with_provision.hex"
  VERBATIM
)

add_custom_target(provision_combined ALL DEPENDS ${PROVISION_COMBINED_FILE})

# Make west flash use the merged hex file instead of the plain zephyr.hex.
set(provision_flash_file "${PROVISION_COMBINED_FILE}")
string(REPLACE "${ZEPHYR_BINARY_DIR}/" "" provision_flash_file "${provision_flash_file}")
get_target_property(yaml_contents runners_yaml_props_target yaml_contents)
string(REGEX REPLACE "\n  hex_file: [^\n]*\n"
                     "\n  hex_file: ${provision_flash_file}\n"
                     yaml_contents "${yaml_contents}")
set_target_properties(runners_yaml_props_target PROPERTIES yaml_contents "${yaml_contents}")

message(STATUS "ZMS Provisioning Configuration:")
message(STATUS "  Storage Base: ${storage_base}")
message(STATUS "  Storage Size: ${storage_size}")
message(STATUS "  Write Block Size: ${write_block_size}")
message(STATUS "  Sector Size: ${ZMS_SECTOR_SIZE}")
message(STATUS "  Provision Output: ${PROVISION_OUTPUT_FILE}")
message(STATUS "  Merged Output: ${PROVISION_COMBINED_FILE}")
