# Texas Instruments AM26x MCU Family
#
# Copyright (c) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_TI_AM26X)
    set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "SoC Linker Script")
    add_subdirectory(${SOC_SERIES})
endif()
