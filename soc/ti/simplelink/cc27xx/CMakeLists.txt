# Copyright (c) 2025 Texas Instruments Incorporated
# Copyright (c) 2024 BayLibre, SAS
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)
zephyr_sources(scfg.c)
zephyr_sources(power.c)
zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

if(NOT CONFIG_FLASH AND CONFIG_GPIO)
zephyr_library()
zephyr_library_sources(../lpf3_common/ext_flash_dpd.c)
endif()

zephyr_linker_sources_ifdef(CONFIG_HAS_TI_CCFG SECTIONS ccfg.ld)

zephyr_linker_sources_ifdef(CONFIG_HAS_TI_SCFG SECTIONS scfg.ld)

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  # Insert CRC32 in CCFG
  COMMAND crc_tool patch-image
    --elf ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
    --symbol-prefix ti_utils_build_GenMap_sym_CRC_CCFG
    --output ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME} ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}

  # Insert CRC32 in SCFG
  COMMAND crc_tool patch-image
    --elf ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
    --symbol-prefix ti_utils_build_GenMap_sym_CRC_SCFG
    --output ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME} ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}
  )

# Register the generated hex file with the west flash runner
set_property(TARGET runners_yaml_props_target PROPERTY hex_file ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME})

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
