# Copyright 2023-2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(.)

zephyr_library_sources(soc.c power.c)
zephyr_library_sources_ifdef(CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS mpu_regions.c)
zephyr_linker_sources(SECTIONS sections.ld)
zephyr_library_sources_ifdef(CONFIG_PLATFORM_SPECIFIC_INIT s32k3xx_startup.S)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
