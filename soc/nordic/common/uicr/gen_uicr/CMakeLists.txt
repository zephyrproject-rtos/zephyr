# SPDX-License-Identifier: Apache-2.0
#
# The code in this CMakeLists.txt constructs the arguments for gen_uicr.py
# and creates a flashable zephyr.hex file containing UICR data (no C code compiled)
#

cmake_minimum_required(VERSION 3.20.0)

# Instead of adding all of Zephyr we add just the subset that is
# required to generate uicr.hex.
#
# The generation of uicr.hex is configured by this image, so we
# include modules from zephyr_default up until kconfig.

find_package(Zephyr
  COMPONENTS zephyr_default:kconfig
  REQUIRED HINTS $ENV{ZEPHYR_BASE}
  )

# Needed since the CMakeLists.txt that would normally set this is not run in this image.
include(${ZEPHYR_BASE}/modules/hal_nordic/ironside/se/ironside_support_dir.cmake)

project(uicr)

# Function to parse a Kconfig value from a .config file
function(parse_kconfig_value config_file config_name output_var)
  file(STRINGS ${config_file} config_lines ENCODING "UTF-8")
  foreach(line ${config_lines})
    # Match quoted strings like CONFIG_FOO="value"
    if("${line}" MATCHES "^${config_name}=\"(.*)\"$")
      set(${output_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
      return()
    endif()
    # Match unquoted values like CONFIG_FOO=y or CONFIG_FOO=n
    if("${line}" MATCHES "^${config_name}=(.*)$")
      set(${output_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
  # If not found, return empty (including "# CONFIG_FOO is not set" case)
  set(${output_var} "" PARENT_SCOPE)
endfunction()

# Function to compute partition absolute address and size from devicetree
function(compute_partition_address_and_size partition_nodelabel output_address_var output_size_var)
  dt_nodelabel(partition_path NODELABEL ${partition_nodelabel} REQUIRED)
  dt_reg_addr(partition_address PATH ${partition_path} REQUIRED)
  dt_reg_size(partition_size PATH ${partition_path} REQUIRED)

  # Set output variables in parent scope
  set(${output_address_var} ${partition_address} PARENT_SCOPE)
  set(${output_size_var} ${partition_size} PARENT_SCOPE)
endfunction()

# Function to compute optional partition address and size from devicetree
# If partition doesn't exist, sets both address and size to 0
function(compute_optional_partition_address_and_size partition_nodelabel output_address_var output_size_var)
  # Initialize with default values
  set(${output_address_var} 0 PARENT_SCOPE)
  set(${output_size_var} 0 PARENT_SCOPE)

  # Check if partition exists
  dt_nodelabel(partition_path NODELABEL ${partition_nodelabel} QUIET)
  if(partition_path)
    # Call nested function with different variable names to avoid naming conflicts
    compute_partition_address_and_size(${partition_nodelabel} temp_addr temp_size)

    # Copy the results to the output variables in parent scope
    set(${output_address_var} ${temp_addr} PARENT_SCOPE)
    set(${output_size_var} ${temp_size} PARENT_SCOPE)
  endif()
endfunction()

set(IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE "")

if(CONFIG_SOC_NRF54H20)
  set(IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE
    "${IRONSIDE_SUPPORT_DIR}/se/resources/periphconf_registers-nrf54h20_xxaa-v23.4.0+27.json"
  )
endif()

# Use CMAKE_VERBOSE_MAKEFILE to silence an unused-variable warning.
if(CMAKE_VERBOSE_MAKEFILE)
endif()

set(lock_args)
set(eraseprotect_args)
set(approtect_args)
set(protectedmem_args)
set(periphconf_args)
set(mpcconf_args)
set(wdtstart_args)
set(periphconf_elfs)
set(mpcconf_elfs)
set(policy_args)
set(merged_hex_file ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.hex)
set(secondary_periphconf_elfs)
set(secondary_mpcconf_elfs)
set(uicr_hex_file ${APPLICATION_BINARY_DIR}/zephyr/uicr.hex)
set(periphconf_hex_file ${APPLICATION_BINARY_DIR}/zephyr/periphconf.hex)
set(secondary_periphconf_hex_file ${APPLICATION_BINARY_DIR}/zephyr/secondary_periphconf.hex)
set(mpcconf_hex_file ${APPLICATION_BINARY_DIR}/zephyr/mpcconf.hex)
set(secondary_mpcconf_hex_file ${APPLICATION_BINARY_DIR}/zephyr/secondary_mpcconf.hex)

# These are always output, others depend on the options
set(gen_uicr_outputs ${merged_hex_file} ${uicr_hex_file})
set(gen_uicr_depends)

# Get UICR absolute address from this image's devicetree
dt_nodelabel(uicr_path NODELABEL "uicr" REQUIRED)
dt_reg_addr(UICR_ADDRESS PATH ${uicr_path} REQUIRED)

# Handle secure storage configuration
set(securestorage_args)
if(CONFIG_GEN_UICR_SECURESTORAGE)
  list(APPEND securestorage_args --securestorage)

  # Extract secure storage partition information (required)
  compute_partition_address_and_size("secure_storage_partition" SECURE_STORAGE_ADDRESS SECURE_STORAGE_SIZE)
  list(APPEND securestorage_args --securestorage-address ${SECURE_STORAGE_ADDRESS})
  list(APPEND securestorage_args --securestorage-size ${SECURE_STORAGE_SIZE})

  # Extract individual partition information for validation (optional partitions)
  compute_optional_partition_address_and_size("cpuapp_crypto_partition" CPUAPP_CRYPTO_ADDRESS CPUAPP_CRYPTO_SIZE)
  compute_optional_partition_address_and_size("cpurad_crypto_partition" CPURAD_CRYPTO_ADDRESS CPURAD_CRYPTO_SIZE)
  compute_optional_partition_address_and_size("cpuapp_its_partition" CPUAPP_ITS_ADDRESS CPUAPP_ITS_SIZE)
  compute_optional_partition_address_and_size("cpurad_its_partition" CPURAD_ITS_ADDRESS CPURAD_ITS_SIZE)

  list(APPEND securestorage_args --cpuapp-crypto-address ${CPUAPP_CRYPTO_ADDRESS})
  list(APPEND securestorage_args --cpuapp-crypto-size ${CPUAPP_CRYPTO_SIZE})
  list(APPEND securestorage_args --cpurad-crypto-address ${CPURAD_CRYPTO_ADDRESS})
  list(APPEND securestorage_args --cpurad-crypto-size ${CPURAD_CRYPTO_SIZE})
  list(APPEND securestorage_args --cpuapp-its-address ${CPUAPP_ITS_ADDRESS})
  list(APPEND securestorage_args --cpuapp-its-size ${CPUAPP_ITS_SIZE})
  list(APPEND securestorage_args --cpurad-its-address ${CPURAD_ITS_ADDRESS})
  list(APPEND securestorage_args --cpurad-its-size ${CPURAD_ITS_SIZE})
endif(CONFIG_GEN_UICR_SECURESTORAGE)

# Handle LOCK configuration
if(CONFIG_GEN_UICR_LOCK)
  list(APPEND lock_args --lock)
endif()

# Handle ERASEPROTECT configuration
if(CONFIG_GEN_UICR_ERASEPROTECT)
  list(APPEND eraseprotect_args --eraseprotect)
endif()

# Handle APPROTECT configuration
if(CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED)
  list(APPEND approtect_args --approtect-application-protected)
endif()

if(CONFIG_GEN_UICR_APPROTECT_RADIOCORE_PROTECTED)
  list(APPEND approtect_args --approtect-radiocore-protected)
endif()

if(CONFIG_GEN_UICR_APPROTECT_CORESIGHT_PROTECTED)
  list(APPEND approtect_args --approtect-coresight-protected)
endif()

# Handle protected memory configuration
if(CONFIG_GEN_UICR_PROTECTEDMEM)
  list(APPEND protectedmem_args --protectedmem)
  list(APPEND protectedmem_args --protectedmem-size-bytes ${CONFIG_GEN_UICR_PROTECTEDMEM_SIZE_BYTES})
endif()

# Handle WDTSTART configuration
if(CONFIG_GEN_UICR_WDTSTART)
  list(APPEND wdtstart_args --wdtstart)
  list(APPEND wdtstart_args --wdtstart-instance-code ${CONFIG_GEN_UICR_WDTSTART_INSTANCE_CODE})
  list(APPEND wdtstart_args --wdtstart-crv ${CONFIG_GEN_UICR_WDTSTART_CRV})
endif()

if(CONFIG_GEN_UICR_MPCCONF)
  list(APPEND mpcconf_args --mpcconf)

  # Compute MPCCONF absolute address and size from this image's devicetree
  compute_partition_address_and_size("mpcconf_partition" MPCCONF_ADDRESS MPCCONF_SIZE)

  list(APPEND mpcconf_args --mpcconf-address ${MPCCONF_ADDRESS})
  list(APPEND mpcconf_args --mpcconf-size ${MPCCONF_SIZE})
endif()

if(CONFIG_GEN_UICR_PERIPHCONF)
  list(APPEND periphconf_args --periphconf)

  # Compute PERIPHCONF absolute address and size from this image's devicetree
  compute_partition_address_and_size("periphconf_partition" PERIPHCONF_ADDRESS PERIPHCONF_SIZE)

  list(APPEND periphconf_args --periphconf-address ${PERIPHCONF_ADDRESS})
  list(APPEND periphconf_args --periphconf-size ${PERIPHCONF_SIZE})
endif()

if(CONFIG_GEN_UICR_GENERATE_PERIPHCONF OR CONFIG_GEN_UICR_SECONDARY_GENERATE_PERIPHCONF OR
   CONFIG_GEN_UICR_GENERATE_MPCCONF OR CONFIG_GEN_UICR_SECONDARY_GENERATE_MPCCONF)
  # gen_uicr.py parses all zephyr.elf files. To find these files (which
  # have not been built yet) we scan sibling build directories for
  # zephyr.dts
  get_filename_component(SYSBUILD_DIR ${APPLICATION_BINARY_DIR} DIRECTORY)
  file(GLOB _siblings LIST_DIRECTORIES true "${SYSBUILD_DIR}/*")
  foreach(_dir ${_siblings})
    get_filename_component(_name ${_dir} NAME)
    if(_name STREQUAL "uicr")
      # This image is an exception to the rule. It has a zephyr.dts, but
      # no zephyr.elf
      continue()
    endif()

    if(EXISTS ${_dir}/zephyr/zephyr.dts)
      # If the PERIPHCONF is configured to remain in the firmware, do not include it here.
      parse_kconfig_value(${_dir}/zephyr/.config CONFIG_NRF_PERIPHCONF_SECTION_STRIP strip_periphconf)
      if(NOT (strip_periphconf STREQUAL "y"))
        continue()
      endif()

      parse_kconfig_value(${_dir}/zephyr/.config CONFIG_NRF_PERIPHCONF_SECTION has_periphconf)
      parse_kconfig_value(${_dir}/zephyr/.config CONFIG_NRF_MPCCONF_SECTION has_mpcconf)

      # Read CONFIG_KERNEL_BIN_NAME from the sibling's .config file
      parse_kconfig_value(${_dir}/zephyr/.config CONFIG_KERNEL_BIN_NAME kernel_bin_name)
      set(kernel_elf_path ${_dir}/zephyr/${kernel_bin_name}.elf)

      # Check if this is secondary firmware by reading the Kconfig from .config
      parse_kconfig_value(${_dir}/zephyr/.config CONFIG_IS_IRONSIDE_SE_SECONDARY_IMAGE is_secondary)
      if(is_secondary STREQUAL "y")
        if(has_periphconf)
          list(APPEND secondary_periphconf_elfs ${kernel_elf_path})
        endif()
        if(has_mpcconf)
          list(APPEND secondary_mpcconf_elfs ${kernel_elf_path})
        endif()
      else()
        if(has_periphconf)
          list(APPEND periphconf_elfs ${kernel_elf_path})
        endif()
        if(has_mpcconf)
          list(APPEND mpcconf_elfs ${kernel_elf_path})
        endif()
      endif()
    endif()
  endforeach()

  if(CONFIG_GEN_UICR_GENERATE_PERIPHCONF)
    # Set up periphconf arguments for gen_uicr.py
    list(APPEND periphconf_args --out-periphconf-hex ${periphconf_hex_file})
    list(APPEND gen_uicr_outputs ${periphconf_hex_file})

    foreach(elf ${periphconf_elfs})
      list(APPEND periphconf_args --in-periphconf-elf ${elf})
      list(APPEND gen_uicr_depends ${elf})
    endforeach()
  endif()
  if(CONFIG_GEN_UICR_GENERATE_MPCCONF)
    # Set up mpcconf arguments for gen_uicr.py
    list(APPEND mpcconf_args --out-mpcconf-hex ${mpcconf_hex_file})
    list(APPEND gen_uicr_outputs ${mpcconf_hex_file})

    foreach(elf ${mpcconf_elfs})
      list(APPEND mpcconf_args --in-mpcconf-elf ${elf})
      list(APPEND gen_uicr_depends ${elf})
    endforeach()
  endif()
endif()

if(CONFIG_GEN_UICR_SECONDARY)
  set(secondary_args --secondary)

  # Compute SECONDARY partition absolute address from this image's devicetree
  compute_partition_address_and_size("secondary_partition" SECONDARY_ADDRESS SECONDARY_SIZE)

  list(APPEND secondary_args
    --secondary-address ${SECONDARY_ADDRESS}
    --secondary-processor ${CONFIG_GEN_UICR_SECONDARY_PROCESSOR_VALUE}
    )

  # Handle secondary WDTSTART configuration
  if(CONFIG_GEN_UICR_SECONDARY_WDTSTART)
    list(APPEND secondary_args --secondary-wdtstart)
    list(APPEND secondary_args --secondary-wdtstart-instance-code ${CONFIG_GEN_UICR_SECONDARY_WDTSTART_INSTANCE_CODE})
    list(APPEND secondary_args --secondary-wdtstart-crv ${CONFIG_GEN_UICR_SECONDARY_WDTSTART_CRV})
  endif()

  # Handle secondary TRIGGER configuration
  if(CONFIG_GEN_UICR_SECONDARY_TRIGGER)
    list(APPEND secondary_args --secondary-trigger)

    # Compute RESETREAS bitmask from individual trigger configs
    set(resetreas_value 0)
    if(CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONWDT0)
      math(EXPR resetreas_value "${resetreas_value} + 0x001")
    endif()
    if(CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONWDT1)
      math(EXPR resetreas_value "${resetreas_value} + 0x002")
    endif()
    if(CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONLOCKUP)
      math(EXPR resetreas_value "${resetreas_value} + 0x008")
    endif()
    if(CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCOREWDT0)
      math(EXPR resetreas_value "${resetreas_value} + 0x020")
    endif()
    if(CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCOREWDT1)
      math(EXPR resetreas_value "${resetreas_value} + 0x040")
    endif()
    if(CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCORELOCKUP)
      math(EXPR resetreas_value "${resetreas_value} + 0x100")
    endif()

    list(APPEND secondary_args --secondary-trigger-resetreas ${resetreas_value})
  endif()

  # Handle secondary PROTECTEDMEM configuration
  if(CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM)
    list(APPEND secondary_args --secondary-protectedmem-size ${CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM_SIZE_BYTES})
  endif()

  # Handle secondary MPCCONF configuration
  if(CONFIG_GEN_UICR_SECONDARY_MPCCONF)
    list(APPEND secondary_args --secondary-mpcconf)

    # Compute SECONDARY_MPCCONF absolute address and size from this image's devicetree
    compute_partition_address_and_size("secondary_mpcconf_partition" SECONDARY_MPCCONF_ADDRESS SECONDARY_MPCCONF_SIZE)

    list(APPEND secondary_args --secondary-mpcconf-address ${SECONDARY_MPCCONF_ADDRESS})
    list(APPEND secondary_args --secondary-mpcconf-size ${SECONDARY_MPCCONF_SIZE})
  endif()

  # Handle secondary PERIPHCONF configuration
  if(CONFIG_GEN_UICR_SECONDARY_PERIPHCONF)
    list(APPEND secondary_args --secondary-periphconf)

    # Compute SECONDARY_PERIPHCONF absolute address and size from this image's devicetree
    compute_partition_address_and_size("secondary_periphconf_partition" SECONDARY_PERIPHCONF_ADDRESS SECONDARY_PERIPHCONF_SIZE)

    list(APPEND secondary_args --secondary-periphconf-address ${SECONDARY_PERIPHCONF_ADDRESS})
    list(APPEND secondary_args --secondary-periphconf-size ${SECONDARY_PERIPHCONF_SIZE})
  endif()

  if(CONFIG_GEN_UICR_SECONDARY_GENERATE_PERIPHCONF)
    list(APPEND secondary_args --out-secondary-periphconf-hex ${secondary_periphconf_hex_file})
    list(APPEND gen_uicr_outputs ${secondary_periphconf_hex_file})

    foreach(elf ${secondary_periphconf_elfs})
      list(APPEND secondary_args --in-secondary-periphconf-elf ${elf})
      list(APPEND gen_uicr_depends ${elf})
    endforeach()
  endif()
  if(CONFIG_GEN_UICR_SECONDARY_GENERATE_MPCCONF)
    list(APPEND secondary_args --out-secondary-mpcconf-hex ${secondary_mpcconf_hex_file})
    list(APPEND gen_uicr_outputs ${secondary_mpcconf_hex_file})

    foreach(elf ${secondary_mpcconf_elfs})
      list(APPEND secondary_args --in-secondary-mpcconf-elf ${elf})
      list(APPEND gen_uicr_depends ${elf})
    endforeach()
  endif()
endif()

if(CONFIG_GEN_UICR_POLICY_PERIPHCONF_STAGE_INIT OR CONFIG_GEN_UICR_POLICY_PERIPHCONF_STAGE_NORMAL)
  list(APPEND policy_args --policy-periphconf-stage ${CONFIG_GEN_UICR_POLICY_PERIPHCONF_STAGE_VALUE})
endif()

if(CONFIG_GEN_UICR_POLICY_MPCCONF_STAGE_INIT OR CONFIG_GEN_UICR_POLICY_MPCCONF_STAGE_NORMAL)
  list(APPEND policy_args --policy-mpcconf-stage ${CONFIG_GEN_UICR_POLICY_MPCCONF_STAGE_VALUE})
endif()

set(periphconf_check_base_cmd
  ${CMAKE_COMMAND} -E env ZEPHYR_BASE=${ZEPHYR_BASE}
  ${PYTHON_EXECUTABLE} ${IRONSIDE_SUPPORT_DIR}/se/tool/ironside/__main__.py
  periphconf-check
  --in-periphconf-registers-json ${IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE}
)

set(periphconf_validate_command)
set(secondary_periphconf_validate_command)

if(CONFIG_GEN_UICR_GENERATE_PERIPHCONF AND IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE)
  set(check_cmd ${periphconf_check_base_cmd} --in-periphconf-hex ${periphconf_hex_file})

  set(periphconf_validate_build_command
    COMMAND ${check_cmd} --mode errors
  )

  add_custom_target(
    periphconf_validate
    COMMAND ${check_cmd} --mode errors
    DEPENDS ${periphconf_hex_file}
    COMMENT "Validating ${periphconf_hex_file}"
  )
  add_custom_target(
    periphconf_dump
    COMMAND ${check_cmd} --mode full
    DEPENDS ${periphconf_hex_file}
    COMMENT "Printing ${periphconf_hex_file}"
  )
  add_custom_target(
    periphconf_dump_raw
    COMMAND ${check_cmd} --mode full --style raw
    DEPENDS ${periphconf_hex_file}
    COMMENT "Printing ${periphconf_hex_file}"
  )
endif()

if(CONFIG_GEN_UICR_SECONDARY_GENERATE_PERIPHCONF AND IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE)
  set(check_cmd ${periphconf_check_base_cmd} --in-periphconf-hex ${secondary_periphconf_hex_file})

  set(secondary_periphconf_validate_build_command
    COMMAND ${check_cmd} --mode errors
  )

  add_custom_target(
    secondary_periphconf_validate
    COMMAND ${check_cmd} --mode errors
    DEPENDS ${secondary_periphconf_hex_file}
    COMMENT "Validating ${secondary_periphconf_hex_file}"
  )
  add_custom_target(
    secondary_periphconf_dump
    COMMAND ${check_cmd} --mode full
    DEPENDS ${secondary_periphconf_hex_file}
    COMMENT "Printing ${secondary_periphconf_hex_file}"
  )
  add_custom_target(
    secondary_periphconf_dump_raw
    COMMAND ${check_cmd} --mode full --style raw
    DEPENDS ${secondary_periphconf_hex_file}
    COMMENT "Printing ${secondary_periphconf_hex_file}"
  )
endif()

add_custom_command(
  OUTPUT ${gen_uicr_outputs}
  COMMAND ${PYTHON_EXECUTABLE} ${IRONSIDE_SUPPORT_DIR}/se/tool/ironside/__main__.py
          gen-uicr
          --uicr-address ${UICR_ADDRESS}
          --out-merged-hex ${merged_hex_file}
          --out-uicr-hex ${uicr_hex_file}
          --periphconf-section-name "periphconf_entry"
          --mpcconf-section-name "mpcconf_entry"
          --periphconf-ipcmap-reallocate
          ${lock_args}
          ${eraseprotect_args}
          ${approtect_args}
          ${wdtstart_args}
          ${mpcconf_args}
          ${periphconf_args}
          ${securestorage_args}
          ${protectedmem_args}
          ${secondary_args}
          ${policy_args}
  ${periphconf_validate_build_command}
  ${secondary_periphconf_validate_build_command}
  DEPENDS ${gen_uicr_depends}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "Generating UICR artifacts"
)

# Add zephyr subdirectory to handle flash configuration with correct paths
add_subdirectory(zephyr)

add_custom_target(gen_uicr ALL DEPENDS ${gen_uicr_outputs})
