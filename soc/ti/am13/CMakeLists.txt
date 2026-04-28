# Texas Instruments AM13 MCU Family
#
# Copyright (c) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_TI_AM13)
    set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "SoC Linker Script")
    add_subdirectory(${SOC_SERIES})
endif()
