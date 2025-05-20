# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(mpu_regions.c)
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_PM power.c)

# CAT1B family defines
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 CY_USING_HAL)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B COMPONENT_CAT1B)
zephyr_compile_definitions(COMPONENT_CM33)

# In MTB for APPTYPE == flash: -DFLASH_BOOT -DCY_PDL_FLASH_BOOT
zephyr_compile_definitions(FLASH_BOOT)
zephyr_compile_definitions(CY_PDL_FLASH_BOOT)

# Use custome linker script
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/cat1b/cyw20829/linker.ld CACHE INTERNAL "")

# Get sram_bootstrap address and size
dt_nodelabel(sram_bootstrap NODELABEL "sram_bootstrap")
dt_reg_addr(bootstrap_dst_addr PATH ${sram_bootstrap})
dt_reg_size(bootstrap_size PATH ${sram_bootstrap})

# Calculate the place in flash
math(EXPR flash_addr_offset
  "${CONFIG_CYW20829_FLASH_SAHB_ADDR} + ${CONFIG_FLASH_LOAD_OFFSET} + ${CONFIG_ROM_START_OFFSET}"
  OUTPUT_FORMAT HEXADECIMAL
)
set(gen_app_header_args --flash_addr_offset ${flash_addr_offset})

# Generate platform specific header (TOC2, l1_desc, etc)
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/soc/infineon/cat1b/cyw20829/gen_app_header.py
    -p ${ZEPHYR_BINARY_DIR} -n ${KERNEL_NAME} ${gen_app_header_args}
    --bootstrap-size ${bootstrap_size}
    --bootstrap-dst-addr ${bootstrap_dst_addr}
  )

set(MERGED_FILE ${CMAKE_BINARY_DIR}/zephyr/zephyr_merged.hex  CACHE PATH "merged hex")

# Merge platform specific header and zephyr image to a single binary.
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
    -o ${MERGED_FILE}
    ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.hex ${ZEPHYR_BINARY_DIR}/app_header.hex
  )

set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${MERGED_FILE})
