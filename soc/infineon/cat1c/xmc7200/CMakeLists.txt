# Copyright (c) 2025 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_compile_definitions_ifdef(CONFIG_SOC_XMC7200D_E272K8384_M7_0 CORE_NAME_CM7_0)
zephyr_compile_definitions_ifdef(CONFIG_SOC_XMC7200D_E272K8384_M7_1 CORE_NAME_CM7_1)

zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M0PLUS soc_m0p.c)
zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M7 soc_m7.c)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

# CAT1C family defines
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 CY_USING_HAL)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1C COMPONENT_CAT1C)
