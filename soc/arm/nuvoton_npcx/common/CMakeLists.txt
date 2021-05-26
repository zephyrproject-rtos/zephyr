# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  scfg.c
)

# Check for disabling header CRC.
if (NOT DEFINED CONFIG_NPCX_HEADER_ENABLE_HEADER_CRC)
    set(NPCX_HEADER_HCRC "-nohcrc")
endif()

# Check for disabling firmware CRC.
if (NOT DEFINED CONFIG_NPCX_HEADER_ENABLE_FIRMWARE_CRC)
    set(NPCX_HEADER_FCRC "-nofcrc")
endif()

if (DEFINED CONFIG_NPCX_HEADER)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/common/ecst/ecst.py
    -i ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
    -o ${NPCX_IMAGE_FILE}
    ${NPCX_HEADER_HCRC} ${NPCX_HEADER_FCRC}
    -chip ${CONFIG_NPCX_HEADER_CHIP}
    -flashsize ${CONFIG_NPCX_HEADER_FLASH_SIZE}
    -spiclkratio ${CONFIG_NPCX_HEADER_CORE_CLOCK_SPI_CLOCK_RATIO}
    -spimaxclk ${CONFIG_NPCX_HEADER_SPI_MAX_CLOCK}
    -spireadmode ${CONFIG_NPCX_HEADER_SPI_READ_MODE}
  )
endif()
