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
  # Get storage partition offset and size
  dt_reg_addr(storage_reg_addr PATH ${storage_partition_node})
  dt_reg_size(storage_size PATH ${storage_partition_node})

  # Get flash device from chosen node
  dt_chosen(flash_node PROPERTY "zephyr,flash")

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
    "zephyr,mapped-partition" IN_LIST storage_partition_compatible
  )
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

# Provisioning configuration
set(zms_sector_size "0x1000")
set(zms_provision_data_file "example_data.txt")
# Inline ID:hexdata pairs, semicolon-separated. IDs must not collide with the
# ones in the input file.
set(zms_provision_data "30:6162;31:63646566")
set(zms_output_name "provisioned_raw.hex")

# Path to the provisioning script
set(provision_script ${ZEPHYR_BASE}/scripts/build/gen_zms_provision_data.py)

# Output directory and files
set(provision_output_dir ${CMAKE_BINARY_DIR}/zephyr)
set(app_hex_file ${provision_output_dir}/zephyr.hex)
set(provision_output_file ${provision_output_dir}/${zms_output_name})

# Build the command line arguments for the provisioning script.
set(provision_args
  -f ${storage_base}
  -s ${storage_size}
  -w ${write_block_size}
  -c ${zms_sector_size}
)

# Add the data file and the individual data pairs
string(REPLACE ";" ";-d;" data_args "${zms_provision_data}")
list(APPEND provision_args -i ${zms_provision_data_file} -d ${data_args})

set(provision_depends ${provision_script})

# Create custom command to generate the standalone provisioning-only image
add_custom_command(
  OUTPUT ${provision_output_file}
  COMMAND ${CMAKE_COMMAND} -E remove -f ${provision_output_file}
  COMMAND ${PYTHON_EXECUTABLE} ${provision_script} ${provision_args} -o ${provision_output_file}
  DEPENDS ${provision_depends}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating ZMS provisioning data: ${zms_output_name}"
  VERBATIM
)

# Create custom target for the provisioning
add_custom_target(provision_raw_data ALL
  DEPENDS ${provision_output_file}
)

# Merge the standalone provisioning image with the application hex, so a
# single west flash call programs both the application and the provisioning
# data.
set(provision_combined_file ${provision_output_dir}/zephyr_with_provision.hex)
set(mergehex_script ${ZEPHYR_BASE}/scripts/build/mergehex.py)

add_custom_command(
  OUTPUT ${provision_combined_file}
  COMMAND ${CMAKE_COMMAND} -E remove -f ${provision_combined_file}
  COMMAND ${PYTHON_EXECUTABLE} ${mergehex_script}
          ${provision_output_file} ${app_hex_file}
          -o ${provision_combined_file}
  DEPENDS ${provision_output_file} ${app_hex_file}
  COMMENT "Merging ZMS provisioning data with application hex: zephyr_with_provision.hex"
  VERBATIM
)

add_custom_target(provision_combined ALL DEPENDS ${provision_combined_file})

# Make west flash use the merged hex file instead of the plain zephyr.hex.
set(provision_flash_file "${provision_combined_file}")
string(REPLACE "${ZEPHYR_BINARY_DIR}/" "" provision_flash_file "${provision_flash_file}")
get_target_property(yaml_contents runners_yaml_props_target yaml_contents)
string(REGEX REPLACE
  "\n  hex_file: [^\n]*\n"
  "\n  hex_file: ${provision_flash_file}\n"
  yaml_contents "${yaml_contents}"
)
set_target_properties(runners_yaml_props_target PROPERTIES yaml_contents "${yaml_contents}")

message(STATUS "ZMS Provisioning Configuration:")
message(STATUS "  Storage Base: ${storage_base}")
message(STATUS "  Storage Size: ${storage_size}")
message(STATUS "  Write Block Size: ${write_block_size}")
message(STATUS "  Sector Size: ${zms_sector_size}")
message(STATUS "  Provision Output: ${provision_output_file}")
message(STATUS "  Merged Output: ${provision_combined_file}")
