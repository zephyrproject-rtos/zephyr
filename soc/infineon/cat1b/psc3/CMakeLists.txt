# Copyright (c) 2025 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_include_directories(.)

# CAT1B family defines
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 CY_USING_HAL)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B COMPONENT_CAT1B)
zephyr_compile_definitions(COMPONENT_CM33)
zephyr_compile_definitions(CORE_NAME_CM33_0)
zephyr_compile_definitions_ifdef(CONFIG_TRUSTED_EXECUTION_SECURE COMPONENT_SECURE_DEVICE)

zephyr_compile_definitions(FLASH_BOOT)
zephyr_compile_definitions(CY_PDL_FLASH_BOOT)
zephyr_compile_definitions(NORMAL_PROVISIONED_LCS)
zephyr_compile_definitions(USER_FLASH_S_SIZE=0x10000)
zephyr_compile_definitions(USER_SRAM_S_SIZE=0x4000)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
