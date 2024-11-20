# Copyright (c) 2021-2024 Atmosic
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TRUSTED_EXECUTION_SECURE)
  zephyr_compile_options(
    -DSECURE_MODE=1
  )
endif()

# Import SPE-related configurations
function(import_spe_kconfig prefix kconfig_fragment)
  # Parse the lines prefixed with 'prefix' in ${kconfig_fragment}
  file(
    STRINGS
    ${kconfig_fragment}
    DOT_CONFIG_LIST
    REGEX "^${prefix}"
    ENCODING "UTF-8"
  )

  foreach (CONFIG ${DOT_CONFIG_LIST})
    # CONFIG could look like: CONFIG_NET_BUF=y

    # Match the first part, the variable name
    string(REGEX MATCH "[^=]+" CONF_VARIABLE_NAME ${CONFIG})

    # Match the second part, variable value
    string(REGEX MATCH "=(.+$)" CONF_VARIABLE_VALUE ${CONFIG})
    # The variable name match we just did included the '=' symbol. To just get the
    # part on the RHS we use match group 1
    set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})

    if("${CONF_VARIABLE_VALUE}" MATCHES "^\"(.*)\"$") # Is surrounded by quotes
      set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})
    endif()

    set("SPE_${CONF_VARIABLE_NAME}" "${CONF_VARIABLE_VALUE}" PARENT_SCOPE)
    list(APPEND keys "${CONF_VARIABLE_NAME}")
  endforeach()

  foreach(outvar ${ARGN})
    set(${outvar} "${keys}" PARENT_SCOPE)
  endforeach()
endfunction()

# Merge SPE and Zephyr app together.
# Strips IDAU bit from SPE to handle signing together
function(merge_spe spe_path spe_load_addr)

  # Extensionless prefix of any output file.
  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})

  # List of additional build byproducts.
  set(byproducts)

  set(S_NS_HEX_FILE ${output}.merged.hex)
  set(S_NS_BIN_FILE ${output}.merged.bin)
  list(APPEND byproducts ${S_NS_HEX_FILE})
  list(APPEND byproducts ${S_NS_BIN_FILE})

  if (CONFIG_ATM_SPLIT_IMG)
     # merge with the RRAM portion of a split NSPE
     set(NS_APP_HEX_FILE ${output}.rram.hex)
  else()
     set(NS_APP_HEX_FILE ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME})
  endif()

  set(S_APP_BIN_FILE ${spe_path}/zephyr/${KERNEL_BIN_NAME})
  set(S_HEX_NO_IDAU_FILE ${spe_path}/zephyr/${KERNEL_NAME}.idau_stripped.hex)
  MATH(EXPR S_ADDR_NO_IDAU "${spe_load_addr} - 0x10000000")

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${CMAKE_OBJCOPY} ${S_APP_BIN_FILE}
      -I binary -O ihex
      --change-addresses ${S_ADDR_NO_IDAU}
      ${S_HEX_NO_IDAU_FILE}
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
      -o ${S_NS_HEX_FILE}
      ${S_HEX_NO_IDAU_FILE}
      ${NS_APP_HEX_FILE}
    COMMAND ${CMAKE_OBJCOPY} ${S_NS_HEX_FILE}
      -I ihex -O binary
      ${S_NS_BIN_FILE}
  )

  # makes west flash choose the merged build output of a given type.
  if(CONFIG_BUILD_OUTPUT_BIN)
    set_target_properties(runners_yaml_props_target PROPERTIES "bin_file" "${S_NS_BIN_FILE}")
  endif()
  if(CONFIG_BUILD_OUTPUT_HEX)
    set_target_properties(runners_yaml_props_target PROPERTIES "hex_file" "${S_NS_HEX_FILE}")
  endif()

  if(CONFIG_BOOTLOADER_MCUBOOT)
    # need to rename files to allow 'west sign' to properly handle signing
    list(APPEND
      byproducts
      ${output}.nspe.bin
      ${output}.nspe.elf
      ${output}.nspe.hex
    )

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${CMAKE_COMMAND} -E copy ${output}.bin ${output}.nspe.bin
      COMMAND ${CMAKE_COMMAND} -E copy ${output}.elf ${output}.nspe.elf
      COMMAND ${CMAKE_COMMAND} -E copy ${output}.hex ${output}.nspe.hex
      COMMAND ${CMAKE_COMMAND} -E copy ${output}.merged.bin ${output}.bin
      COMMAND ${CMAKE_COMMAND} -E copy ${output}.merged.hex ${output}.hex
    )
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})
endfunction()

# Split Image into RRAM and FLASH parts.
function(split_img)
  # Extensionless prefix of any output file.
  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})

  set(NS_APP_ELF_FILE ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME})

  # List of additional build byproducts.
  set(byproducts)

  set(RRAM_HEX_FILE ${output}.rram.hex)
  set(FLASH_HEX_FILE ${output}.flash.hex)
  list(APPEND byproducts ${RRAM_HEX_FILE})
  list(APPEND byproducts ${FLASH_HEX_FILE})

  set(RRAM_BIN_FILE ${output}.rram.bin)
  set(FLASH_BIN_FILE ${output}.flash.bin)
  list(APPEND byproducts ${RRAM_BIN_FILE})
  list(APPEND byproducts ${FLASH_BIN_FILE})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${CMAKE_OBJCOPY} -j fast_code -O ihex ${NS_APP_ELF_FILE} ${RRAM_HEX_FILE}
    COMMAND ${CMAKE_OBJCOPY} -R fast_code -O ihex ${NS_APP_ELF_FILE} ${FLASH_HEX_FILE}

    COMMAND ${CMAKE_OBJCOPY} -j fast_code -O binary ${NS_APP_ELF_FILE} ${RRAM_BIN_FILE}
    COMMAND ${CMAKE_OBJCOPY} -R fast_code -O binary ${NS_APP_ELF_FILE} ${FLASH_BIN_FILE}
  )

  if(CONFIG_BUILD_OUTPUT_HEX)
    # atmisp can parse comma-delimited strings and handle them independently.
    set_target_properties(runners_yaml_props_target PROPERTIES "hex_file" "${RRAM_HEX_FILE},${FLASH_HEX_FILE}")
  endif()

  if(CONFIG_BUILD_OUTPUT_BIN)
    # atmisp can parse comma-delimited strings and handle them independently.
    set_target_properties(runners_yaml_props_target PROPERTIES "bin_file" "${RRAM_BIN_FILE},${FLASH_BIN_FILE}")
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})
endfunction()

function(atm_zephyr_runner_file type path)
  # Property magic which makes west flash choose the signed build
  # output of a given type.
  set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
endfunction()

# sign the FLASH located NSPE image
function(sign_flash_nspe_tasks)

  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})
  set(FLASH_NSPE_BASE ${output}.flash)
  set(SIGNED_FLASH_NSPE_BASE ${output}.signed.flash)

  # Tread carefully here...
  # Zephyr does not support multi-image signing in mcuboot.cmake
  # minimal west sign functionality is pulled in here to just sign the FLASH NSPE.
  # copied from zephyr/cmake/mcuboot.cmake starts here:
  #  -) Removed encrypted images support
  ##########################################################

  set(keyfile "${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}")
  set(keyfile_enc "${CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE}")

  foreach(file keyfile)
    if(NOT "${${file}}" STREQUAL "")
      if(NOT IS_ABSOLUTE "${${file}}")
        # Relative paths are relative to 'west topdir'.
        set(${file} "${WEST_TOPDIR}/${${file}}")
      endif()

      if(NOT EXISTS "${${file}}" AND NOT "${CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE}")
        message(FATAL_ERROR "west sign can't find file ${${file}} (Note: Relative paths are relative to the west workspace topdir \"${WEST_TOPDIR}\")")
      elseif(NOT (CONFIG_BUILD_OUTPUT_BIN OR CONFIG_BUILD_OUTPUT_HEX))
        message(FATAL_ERROR "Can't sign images for MCUboot: Neither CONFIG_BUILD_OUTPUT_BIN nor CONFIG_BUILD_OUTPUT_HEX is enabled, so there's nothing to sign.")
      endif()
    endif()
  endforeach()

  # Find imgtool. Even though west is installed, imgtool might not be.
  # The user may also have a custom manifest which doesn't include
  # MCUboot.
  #
  # Therefore, go with an explicitly installed imgtool first, falling
  # back on mcuboot/scripts/imgtool.py.
  if(IMGTOOL)
    set(imgtool_path "${IMGTOOL}")
  elseif(DEFINED ZEPHYR_MCUBOOT_MODULE_DIR)
    set(IMGTOOL_PY "${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/imgtool.py")
    if(EXISTS "${IMGTOOL_PY}")
      set(imgtool_path "${IMGTOOL_PY}")
    endif()
  endif()

  # No imgtool, no signed binaries.
  if(NOT DEFINED imgtool_path)
    message(FATAL_ERROR "Can't sign images for MCUboot: can't find imgtool")
    return()
  endif()

  # Basic 'west sign' command and output format independent arguments.
  separate_arguments(west_sign_extra UNIX_COMMAND ${CONFIG_MCUBOOT_CMAKE_WEST_SIGN_PARAMS})
  set(west_sign ${WEST} sign ${west_sign_extra}
    --tool imgtool
    --tool-path "${imgtool_path}"
    --build-dir "${APPLICATION_BINARY_DIR}"
    --hex_file "${FLASH_NSPE_BASE}.hex"
    --bin_file "${FLASH_NSPE_BASE}.bin"
    --shex "${SIGNED_FLASH_NSPE_BASE}.hex"
    --sbin "${SIGNED_FLASH_NSPE_BASE}.bin"
    --image_num 1)

  # Arguments to imgtool.
  if(NOT CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS STREQUAL "")
    # Separate extra arguments into the proper format for adding to
    # extra_post_build_commands.
    #
    # Use UNIX_COMMAND syntax for uniform results across host
    # platforms.
    separate_arguments(imgtool_extra UNIX_COMMAND ${CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS})
  else()
    set(imgtool_extra)
  endif()

  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_extra --key "${keyfile}" ${imgtool_extra})
  endif()

  if(CONFIG_BOOT_MAX_IMG_SECTORS)
    set(imgtool_extra --max-sectors "${CONFIG_BOOT_MAX_IMG_SECTORS}" ${imgtool_extra})
  endif()

  # Use overwrite-only instead of swap upgrades.
  if(CONFIG_MCUBOOT_IMGTOOL_OVERWRITE_ONLY)
    set(imgtool_extra --overwrite-only --align 1 ${imgtool_extra})
  endif()

  set(imgtool_args -- ${imgtool_extra})

  # Extensionless prefix of any output file.
  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})

  # List of additional build byproducts.
  set(byproducts)

  # 'west sign' arguments for confirmed, unconfirmed and encrypted images.
  set(unconfirmed_args)
  set(confirmed_args)
  set(encrypted_args)

  # Set up .bin outputs.
  if(CONFIG_BUILD_OUTPUT_BIN)
    list(APPEND unconfirmed_args --bin --sbin ${SIGNED_FLASH_NSPE_BASE}.bin)
    list(APPEND byproducts ${SIGNED_FLASH_NSPE_BASE}.bin)
    atm_zephyr_runner_file(bin ${SIGNED_FLASH_NSPE_BASE}.bin)
    set(BYPRODUCT_KERNEL_SIGNED_BIN_NAME "${SIGNED_FLASH_NSPE_BASE}.bin"
        CACHE FILEPATH "Signed kernel bin file" FORCE
    )

    if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
      list(APPEND confirmed_args --bin --sbin ${SIGNED_FLASH_NSPE_BASE}.confirmed.bin)
      list(APPEND byproducts ${SIGNED_FLASH_NSPE_BASE}.confirmed.bin)
      set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_BIN_NAME "${SIGNED_FLASH_NSPE_BASE}.confirmed.bin"
          CACHE FILEPATH "Signed and confirmed kernel bin file" FORCE
      )
    endif()

    if(NOT "${keyfile_enc}" STREQUAL "")
      message(FATAL_ERROR "Encrypting NSPE image in Flash not supported")
    endif()
  endif()

  # Set up .hex outputs.
  if(CONFIG_BUILD_OUTPUT_HEX)
    list(APPEND unconfirmed_args --hex --shex ${SIGNED_FLASH_NSPE_BASE}.hex)
    list(APPEND byproducts ${SIGNED_FLASH_NSPE_BASE}.hex)
    atm_zephyr_runner_file(hex ${SIGNED_FLASH_NSPE_BASE}.hex)
    set(BYPRODUCT_KERNEL_SIGNED_HEX_NAME "${SIGNED_FLASH_NSPE_BASE}.hex"
        CACHE FILEPATH "Signed kernel hex file" FORCE
    )

    if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
      list(APPEND confirmed_args --hex --shex ${SIGNED_FLASH_NSPE_BASE}.confirmed.hex)
      list(APPEND byproducts ${SIGNED_FLASH_NSPE_BASE}.confirmed.hex)
      set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_HEX_NAME "${SIGNED_FLASH_NSPE_BASE}.confirmed.hex"
          CACHE FILEPATH "Signed and confirmed kernel hex file" FORCE
      )
    endif()

    if(NOT "${keyfile_enc}" STREQUAL "")
      message(FATAL_ERROR "Encrypting NSPE image in Flash not supported")
    endif()
  endif()

  # Add the west sign calls and their byproducts to the post-processing
  # steps for zephyr.elf.
  #
  # CMake guarantees that multiple COMMANDs given to
  # add_custom_command() are run in order, so adding the 'west sign'
  # calls to the "extra_post_build_commands" property ensures they run
  # after the commands which generate the unsigned versions.
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
    ${west_sign} ${unconfirmed_args} ${imgtool_args})
  if(confirmed_args)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${west_sign} ${confirmed_args} ${imgtool_args} --pad --confirm)
  endif()
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})

  ###########################################################
  # end of copied rules from mcuboot.cmake

endfunction()

# Generate partition_info.map
function(atm_partition_info)
  if(EXISTS "${CONFIG_GEN_PARTITION_INFO_SCRIPT}")
    # Extensionless prefix of any output file.
    set(output_file ${ZEPHYR_BINARY_DIR}/partition_info.map)
    set(dt_file ${ZEPHYR_BINARY_DIR}/zephyr.dts)
    set(EXT_ARGS)
    if(CONFIG_BOOTLOADER_MCUBOOT)
      list(APPEND EXT_ARGS "--use_mcuboot")
    endif()
    if(CONFIG_BOOT_MAX_IMG_SECTORS)
      list(APPEND EXT_ARGS "--max_sector_size")
      list(APPEND EXT_ARGS "${CONFIG_BOOT_MAX_IMG_SECTORS}")
    endif()
    if(CONFIG_ROM_START_OFFSET)
      list(APPEND EXT_ARGS "--rom_start")
      list(APPEND EXT_ARGS "${CONFIG_ROM_START_OFFSET}")
    endif()
    if(CONFIG_ROM_SIZE)
      list(APPEND EXT_ARGS "--rom_size")
      list(APPEND EXT_ARGS "${CONFIG_ROM_SIZE}")
    endif()
    if(CONFIG_USER_DATA_OFFSET)
      list(APPEND EXT_ARGS "--user_data_offset")
      list(APPEND EXT_ARGS "${CONFIG_USER_DATA_OFFSET}")
    endif()
    if(USER_DATA_SIZE)
      list(APPEND EXT_ARGS "--user_data_size")
      list(APPEND EXT_ARGS "${USER_DATA_SIZE}")
    endif()
    if(CONFIG_MERGE_SPE_NSPE)
      list(APPEND EXT_ARGS "--merge_spe_nspe")
    endif()
    if (CONFIG_SOC_ATM34XX_2)
      list(APPEND EXT_ARGS "--revision")
      list(APPEND EXT_ARGS "rev-2")
    endif ()
    if (CONFIG_SOC_ATM34XX_5)
      list(APPEND EXT_ARGS "--revision")
      list(APPEND EXT_ARGS "rev-5")
    endif ()
    if (CONFIG_ATM_SPLIT_IMG)
      list(APPEND EXT_ARGS "--split_img")
      list(APPEND EXT_ARGS "2")
    endif ()
    message("Generate Partition Info Map File to ${output_file}")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${CONFIG_GEN_PARTITION_INFO_SCRIPT} gen -i ${dt_file} -o ${output_file}
        -p ${CONFIG_SOC} -f ${CONFIG_SOC_SERIES} -b ${BOARD} ${EXT_ARGS}
    )
    if (CONFIG_TRUSTED_EXECUTION_NONSECURE AND CONFIG_SPE_PATH)
	set(input_file2 ${spe_path}/zephyr/partition_info.map)
        if(EXISTS "${input_file2}")
	    set(output_file_merge ${ZEPHYR_BINARY_DIR}/partition_info.map.merge)
	    message("Merge Partition Info Map File to ${output_file_merge}")
	    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
	    COMMAND ${PYTHON_EXECUTABLE} ${CONFIG_GEN_PARTITION_INFO_SCRIPT} merge -i1 ${input_file2} -i2 ${output_file} -o ${output_file_merge}
	    )
	endif()
    endif()
  else()
    message("west build can't find ${CONFIG_GEN_PARTITION_INFO_SCRIPT}, ignore generate partition info")
  endif()
endfunction()

# Generate tag data bin files
function(atm_generate_tag_data_bin_file)
  if(EXISTS "${CONFIG_GEN_TAG_DATA_BIN_FILE_SCRIPT}")
    set(partition_file ${ZEPHYR_BINARY_DIR}/partition_info.map)
    set(settings_yaml_file ${CMAKE_SOURCE_DIR}/tag_data/settings.yml)
    set(factory_yaml_file ${CMAKE_SOURCE_DIR}/tag_data/factory.yml)
    if(EXISTS "${settings_yaml_file}")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${CONFIG_GEN_TAG_DATA_BIN_FILE_SCRIPT} -t settings -z ${CONFIG_ZSG_TOOL_PATH} -o ${CMAKE_OBJCOPY} -p ${partition_file} -y ${settings_yaml_file} -b ${ZEPHYR_BINARY_DIR}/zephyr_settings.bin
      )
    else()
      message("ignore generate settings.bin")
    endif()
    if(EXISTS "${factory_yaml_file}")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${CONFIG_GEN_TAG_DATA_BIN_FILE_SCRIPT} -t factory -z ${CONFIG_ZSG_TOOL_PATH} -o ${CMAKE_OBJCOPY} -p ${partition_file} -y ${factory_yaml_file} -b ${ZEPHYR_BINARY_DIR}/zephyr_factory.bin
      )
    else()
      message("ignore generate factory.bin")
    endif()
  else()
    message("west build can't find 'CONFIG_GEN_TAG_DATA_BIN_FILE_SCRIPT', ignore generate tag data bin file")
  endif()
endfunction()

# Handle additional atm34 tasks
function(zephyr_atmosic_tasks)
  if (CONFIG_TRUSTED_EXECUTION_NONSECURE AND CONFIG_SPE_PATH)
    set(spe_path "${CONFIG_SPE_PATH}")
    if(NOT IS_ABSOLUTE "${spe_path}")
      # Relative paths are relative to 'west topdir'.
      set(spe_path "${WEST_TOPDIR}/${spe_path}")
    endif()
    if(NOT EXISTS "${spe_path}")
      message(FATAL_ERROR "west build can't find dir ${spe_path} (Note: Relative paths are relative to the west workspace topdir \"${WEST_TOPDIR}\")")
    endif()

    # Extensionless prefix of any output file.
    set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})
    # Process SPE configs
    set(SPEDOTCONFIG ${spe_path}/zephyr/.config)
    import_spe_kconfig(CONFIG_ ${SPEDOTCONFIG})
    assert(${SPE_CONFIG_TRUSTED_EXECUTION_SECURE} "Error: Application passed as CONFIG_SPE_PATH is not configured as a SPE.")

    # copy the veneer library to a location the zephyr can link
    zephyr_file_copy(${spe_path}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME} ${CMAKE_BINARY_DIR}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME})
    if (CONFIG_ATM_SPLIT_IMG)
      split_img()
      if (CONFIG_BOOTLOADER_MCUBOOT)
        # add tasks to sign the FLASH portion of the NSPE
        sign_flash_nspe_tasks()
      endif()
    endif()
    if (CONFIG_MERGE_SPE_NSPE)
      MATH(EXPR S_LOAD_OFFSET "${SPE_CONFIG_FLASH_BASE_ADDRESS} + ${SPE_CONFIG_FLASH_LOAD_OFFSET}")
      merge_spe(${spe_path} ${S_LOAD_OFFSET})
    endif()
  endif()
  if (CONFIG_GEN_PARTITION_INFO)
    atm_partition_info()
    if(CONFIG_ATM_SETTINGS AND NOT CONFIG_AUTO_TEST)
      atm_generate_tag_data_bin_file()
    endif()
  endif()
endfunction()

zephyr_atmosic_tasks()
