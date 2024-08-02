# Copyright (c) 2023 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(app_header.c)
zephyr_include_directories(.)

# CAT1B family defines
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 CY_USING_HAL)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B COMPONENT_CAT1B)
zephyr_compile_definitions(COMPONENT_CM33)

# In MTB for APPTYPE == flash: -DFLASH_BOOT -DCY_PDL_FLASH_BOOT
zephyr_compile_definitions(FLASH_BOOT)
zephyr_compile_definitions(CY_PDL_FLASH_BOOT)

# Use custome linker script
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/cat1b/cyw20829/linker.ld CACHE INTERNAL "")
