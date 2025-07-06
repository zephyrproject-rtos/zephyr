# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_PM power.c)
zephyr_sources(
  scfg.c
  ${CONFIG_NPCX_SOC_VARIANT}/registers.c
)
zephyr_include_directories(${CONFIG_NPCX_SOC_VARIANT}/include)

# Check for disabling header CRC.
if (NOT DEFINED CONFIG_NPCX_HEADER_ENABLE_HEADER_CRC)
    set(NPCX_HEADER_HCRC "-nohcrc")
endif()

# Check for disabling firmware CRC.
if (NOT DEFINED CONFIG_NPCX_HEADER_ENABLE_FIRMWARE_CRC)
    set(NPCX_HEADER_FCRC "-nofcrc")
endif()

if (DEFINED CONFIG_NPCX_IMAGE_OUTPUT_BIN)
  set(NPCX_BIN_NAME ${CONFIG_KERNEL_BIN_NAME}.npcx.bin)
  string(TOUPPER "${SOC_NAME}" soc_name_upper)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${SOC_${soc_name_upper}_DIR}/common/ecst/ecst.py
    -i ${KERNEL_BIN_NAME}
    -o ${NPCX_BIN_NAME}
    ${NPCX_HEADER_HCRC} ${NPCX_HEADER_FCRC}
    -chip ${CONFIG_NPCX_HEADER_CHIP}
    -flashsize ${CONFIG_NPCX_HEADER_FLASH_SIZE}
    -spiclkratio ${CONFIG_NPCX_HEADER_CORE_CLOCK_SPI_CLOCK_RATIO}
    -spimaxclk ${CONFIG_NPCX_HEADER_SPI_MAX_CLOCK}
    -spireadmode ${CONFIG_NPCX_HEADER_SPI_READ_MODE}
  )
  if (DEFINED CONFIG_NPCX_IMAGE_OUTPUT_HEX)
    set(NPCX_HEX_NAME ${CONFIG_KERNEL_BIN_NAME}.npcx.hex)
    # Property magic which makes west flash choose right file.
    set_property(TARGET runners_yaml_props_target PROPERTY hex_file "${CONFIG_KERNEL_BIN_NAME}.npcx.hex")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_intarget>binary
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>ihex
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${NPCX_BIN_NAME}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${NPCX_HEX_NAME}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
    )
  endif()
endif()
