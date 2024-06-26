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

# Handle additional atmx2 tasks
function(zephyr_atmosic_tasks)
  if (CONFIG_GEN_PARTITION_INFO)
    atm_partition_info()
  endif()
endfunction()

zephyr_atmosic_tasks()
