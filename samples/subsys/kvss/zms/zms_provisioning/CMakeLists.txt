#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zms_provisioning)

target_sources(app PRIVATE src/main.c)

# Extract configuration from device tree
# Storage partition node (labeled as storage_partition)
dt_nodelabel(storage_partition_node NODELABEL "storage_partition")

if(DEFINED storage_partition_node)
    # Get storage partition address and size
    dt_reg_addr(storage_base PATH ${storage_partition_node})
    dt_reg_size(storage_size PATH ${storage_partition_node})

    # Get flash device from chosen node
    dt_chosen(flash_node PROPERTY "zephyr,flash")

    if(NOT DEFINED flash_node)
        # Fallback: try to get flash device by label
        dt_nodelabel(flash_node NODELABEL "mram1x")
    endif()

    if(DEFINED flash_node)
        # Get NVM base address and size from flash device
        dt_reg_addr(nvm_address PATH ${flash_node})
        dt_reg_size(nvm_size PATH ${flash_node})

        # Get write block size from flash device
        dt_prop(write_block_size PATH ${flash_node} PROPERTY "write-block-size")
    else()
        message(WARNING "Flash device node not found. Using defaults for NVM settings.")
        set(nvm_address "0x0")
        set(nvm_size "0x0")
        set(write_block_size "16")
    endif()

    set(ZMS_STORAGE_BASE "${storage_base}")
    set(ZMS_STORAGE_SIZE "${storage_size}")
    set(ZMS_NVM_ADDRESS "${nvm_address}")
    set(ZMS_NVM_SIZE "${nvm_size}")
    set(ZMS_WRITE_BLOCK_SIZE "${write_block_size}")

    message(STATUS "Device tree configuration found:")
    message(STATUS "  Storage partition: ${storage_partition_node}")
    if(DEFINED flash_node)
        message(STATUS "  Flash device: ${flash_node}")
    endif()
else()
    message(WARNING "Device tree node 'storage_partition' not found. Using default values.")
    # Fallback to CMake cache variables
    set(ZMS_STORAGE_BASE "0x0" CACHE STRING "Storage base address in hex format")
    set(ZMS_STORAGE_SIZE "0x1000" CACHE STRING "Storage size in bytes in hex format")
    set(ZMS_NVM_ADDRESS "0x0" CACHE STRING "NVM base address offset in hex format")
    set(ZMS_NVM_SIZE "0x0" CACHE STRING "NVM size in bytes in hex format")
    set(ZMS_WRITE_BLOCK_SIZE "16" CACHE STRING "Write block size in bytes")
endif()

# Other configuration variables
set(ZMS_SECTOR_SIZE "0x1000" CACHE STRING "Sector size in bytes in hex format")
set(ZMS_PROVISION_DATA_FILE "example_data.txt" CACHE STRING "Optional input file with ID-data pairs")
set(ZMS_PROVISION_DATA "" CACHE STRING "Optional ID-data pairs (semicolon-separated)")
if(CONFIG_SAMPLE_ZMS_PROVISIONING_MERGE_WITH_APP_HEX)
    set(ZMS_DEFAULT_OUTPUT_NAME "zephyr_with_provision.hex")
else()
    set(ZMS_DEFAULT_OUTPUT_NAME "provisioned_raw.hex")
endif()

set(ZMS_OUTPUT_NAME "${ZMS_DEFAULT_OUTPUT_NAME}" CACHE STRING
    "Output hex file name; merged or raw depending on CONFIG_SAMPLE_ZMS_PROVISIONING_MERGE_WITH_APP_HEX")

# Path to the provisioning script
set(PROVISION_SCRIPT ${ZEPHYR_BASE}/scripts/build/gen_zms_provision_data.py)

# Output directory and files
set(PROVISION_OUTPUT_DIR ${CMAKE_BINARY_DIR}/zephyr)
set(APP_HEX_FILE ${PROVISION_OUTPUT_DIR}/zephyr.hex)
set(PROVISION_OUTPUT_FILE ${PROVISION_OUTPUT_DIR}/${ZMS_OUTPUT_NAME})

# Build the command line arguments for the provisioning script
set(PROVISION_ARGS
    -f ${ZMS_STORAGE_BASE}
    -s ${ZMS_STORAGE_SIZE}
    -n ${ZMS_NVM_ADDRESS}
    -z ${ZMS_NVM_SIZE}
    -w ${ZMS_WRITE_BLOCK_SIZE}
    -c ${ZMS_SECTOR_SIZE}
)

if(CONFIG_SAMPLE_ZMS_PROVISIONING_MERGE_WITH_APP_HEX)
    list(APPEND PROVISION_ARGS -x ${APP_HEX_FILE})
endif()

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

set(PROVISION_DEPENDS
    ${PROVISION_SCRIPT}
    ${ZEPHYR_BASE}/scripts/build/fs_raw_instance.py
    ${ZEPHYR_BASE}/scripts/build/fs_class.py
    ${ZEPHYR_BASE}/scripts/build/zms.py
)

if(CONFIG_SAMPLE_ZMS_PROVISIONING_MERGE_WITH_APP_HEX)
    list(APPEND PROVISION_DEPENDS ${APP_HEX_FILE})
endif()

# Create custom command to generate provisioned hex file
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

# Configure west flash to use the appropriate hex file.
# In merge mode the provisioning script already produces a merged hex;
# in non-merge mode we merge zephyr.hex + provisioned_raw.hex here so that
# a single west flash call programs both the application and provisioning data.
if(CONFIG_SAMPLE_ZMS_PROVISIONING_MERGE_WITH_APP_HEX)
    # In merge mode, the provisioning script already merged the app hex.
    set(_provision_flash_file "${PROVISION_OUTPUT_FILE}")
else()
    # Combine provisioned_raw.hex and zephyr.hex into one file for west flash.
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

    set(_provision_flash_file "${PROVISION_COMBINED_FILE}")
endif()

# Patch yaml_contents directly to override the default hex_file entry.
get_target_property(_yaml runners_yaml_props_target yaml_contents)
string(REPLACE "  hex_file: ${KERNEL_HEX_NAME}"
               "  hex_file: ${_provision_flash_file}"
               _yaml "${_yaml}")
set_target_properties(runners_yaml_props_target PROPERTIES yaml_contents "${_yaml}")

message(STATUS "ZMS Provisioning Configuration:")
message(STATUS "  Storage Base: ${ZMS_STORAGE_BASE}")
message(STATUS "  Storage Size: ${ZMS_STORAGE_SIZE}")
message(STATUS "  NVM Address: ${ZMS_NVM_ADDRESS}")
message(STATUS "  NVM Size: ${ZMS_NVM_SIZE}")
message(STATUS "  Write Block Size: ${ZMS_WRITE_BLOCK_SIZE}")
message(STATUS "  Sector Size: ${ZMS_SECTOR_SIZE}")
message(STATUS "  Merge With App Hex: ${CONFIG_SAMPLE_ZMS_PROVISIONING_MERGE_WITH_APP_HEX}")
message(STATUS "  Provision Output: ${PROVISION_OUTPUT_FILE}")
