# Copyright (c) 2023 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(app_header.c)
zephyr_include_directories(.)

zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 CY_USING_HAL)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B COMPONENT_CAT1B)
zephyr_compile_definitions(COMPONENT_CM33)

# Use custom linker script
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/cat1b/cyw20829/linker.ld CACHE INTERNAL "")
