# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS mpu_regions.c)
zephyr_linker_sources(SECTIONS sections.ld)
zephyr_library_sources_ifdef(CONFIG_SOC_EARLY_RESET_HOOK s32k5xx_startup.S)
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
