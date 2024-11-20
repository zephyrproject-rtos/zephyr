# Copyright (c) 2021-2024 Atmosic
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(board.c pinmux.c)

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
    message("Generate Partition Info Map File to ${output_file}")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${CONFIG_GEN_PARTITION_INFO_SCRIPT} gen -i ${dt_file} -o ${output_file}
        -p ${CONFIG_SOC} -f ${CONFIG_SOC_SERIES} -b ${BOARD} ${EXT_ARGS}
    )
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

# Handle additional atmx2 tasks
function(zephyr_atmosic_tasks)
  if (CONFIG_GEN_PARTITION_INFO)
    atm_partition_info()
    if(CONFIG_ATM_SETTINGS AND NOT CONFIG_AUTO_TEST)
      atm_generate_tag_data_bin_file()
    endif()
  endif()
endfunction()

zephyr_atmosic_tasks()
