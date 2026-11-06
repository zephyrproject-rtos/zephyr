# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(soc.c)
zephyr_library_sources(boot_header.S)

# Sorts after the start of the ROM region and before the vector table
zephyr_linker_sources(ROM_START SORT_KEY 0x0bk boot_header.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

if(CONFIG_BUILD_OUTPUT_BIN)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/bkimage.py
    ${KERNEL_BIN_NAME}
    ${KERNEL_NAME}.crc.bin
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
    ${PROJECT_BINARY_DIR}/${KERNEL_NAME}.crc.bin
  )

  # zephyr.bin is not flashable, so "west flash" writes the image with the CRC words
  set_target_properties(runners_yaml_props_target PROPERTIES
    bin_file ${PROJECT_BINARY_DIR}/${KERNEL_NAME}.crc.bin
  )
endif()
