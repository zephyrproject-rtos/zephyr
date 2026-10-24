#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zms_provisioning)

target_sources(app PRIVATE src/main.c)

dt_nodelabel(storage_partition_node NODELABEL "storage_partition")

if(DEFINED storage_partition_node)
  dt_reg_addr(storage_reg_addr PATH ${storage_partition_node})
  dt_reg_size(storage_size PATH ${storage_partition_node})

  dt_chosen(flash_node PROPERTY "zephyr,flash")

  if(DEFINED flash_node)
    # Get NVM base address from flash device. This is only needed here, to
    # resolve the partition's absolute address below; the provisioning
    # script itself only ever deals with the storage area.
    dt_reg_addr(nvm_address PATH ${flash_node})

    dt_prop(write_block_size PATH ${flash_node} PROPERTY "write-block-size")
    dt_prop(erase_block_size PATH ${flash_node} PROPERTY "erase-block-size")
  else()
    message(WARNING "Flash device node not found. Using defaults for NVM settings.")
    set(nvm_address "0x0")
    set(write_block_size "16")
    set(erase_block_size "4096")
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
  set(write_block_size "16")
  set(erase_block_size "4096")
  # ZMS needs at least 2 sectors
  math(EXPR storage_size "2 * ${erase_block_size}" OUTPUT_FORMAT HEXADECIMAL)
endif()

# Provisioning configuration
# ZMS sector size, in hex. It must be the same as the sector size the
# application mounts ZMS with (fs.sector_size in src/main.c), which the flash
# driver fills from erase-block-size. A custom value can be used: it must be a
# multiple of the write-block-size for devices that do not need an erase, and
# a multiple of the erase-block-size for devices that do.
math(EXPR zms_sector_size "${erase_block_size}" OUTPUT_FORMAT HEXADECIMAL)
set(zms_provision_data_file "example_data.txt")
# Inline ID:hexdata pairs, semicolon-separated. IDs must not collide with the
# ones in the input file.
set(zms_provision_data "30:6162;31:63646566")
set(zms_output_name "provisioned_raw.hex")

set(provision_script ${ZEPHYR_BASE}/scripts/build/gen_zms_provision_data.py)

set(provision_output_dir ${CMAKE_BINARY_DIR}/zephyr)
set(app_hex_file ${provision_output_dir}/zephyr.hex)
set(provision_output_file ${provision_output_dir}/${zms_output_name})

set(provision_args
  -f ${storage_base}
  -s ${storage_size}
  -w ${write_block_size}
  -c ${zms_sector_size}
)

# The ID size must match the one the application is built with.
if(CONFIG_ZMS_ID_64BIT)
  list(APPEND provision_args --id-size 64)
endif()

list(TRANSFORM zms_provision_data PREPEND "-d;")
list(APPEND provision_args -i ${zms_provision_data_file} ${zms_provision_data})

set(provision_depends ${provision_script} ${CMAKE_CURRENT_SOURCE_DIR}/${zms_provision_data_file})

add_custom_command(
  OUTPUT ${provision_output_file}
  COMMAND ${CMAKE_COMMAND} -E remove -f ${provision_output_file}
  COMMAND ${PYTHON_EXECUTABLE} ${provision_script} ${provision_args} -o ${provision_output_file}
  DEPENDS ${provision_depends}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating ZMS provisioning data: ${zms_output_name}"
  VERBATIM
)

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
add_dependencies(provision_combined zephyr_final)

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
