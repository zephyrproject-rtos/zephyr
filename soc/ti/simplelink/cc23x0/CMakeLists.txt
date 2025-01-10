# Copyright (c) 2024 Texas Instruments Incorporated
# Copyright (c) 2024 BayLibre, SAS
# Copyright (c) 2025 Fabian Pflug
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)
zephyr_sources(power.c)
zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_linker_sources_ifdef(CONFIG_HAS_TI_CCFG SECTIONS ccfg.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

set(checksums_calculated_file ${PROJECT_BINARY_DIR}/${KERNEL_NAME}_crc32.hex  CACHE PATH "crc checksums inserted")

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    # Create a new file with correct crc32 checksums
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/ti_ccfg_crc32_calculate.py
    ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
    ${checksums_calculated_file}
)

# runners_yaml_props_target controls the file used by "west flash"
set_property(TARGET runners_yaml_props_target PROPERTY hex_file ${checksums_calculated_file})

set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${checksums_calculated_file})

# TODO: Replace python script above with crc_tool
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  # Insert CRC32 in CCFG
  COMMAND crc_tool patch-image
	  --elf ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf
	  --symbol-prefix CRC_CCFG
	  --output ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf
  COMMAND ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-objcopy -O ihex ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf ${CMAKE_BINARY_DIR}/zephyr/zephyr.hex
  )
