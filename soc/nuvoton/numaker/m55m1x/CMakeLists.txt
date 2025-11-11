# Copyright (c) 2025 Nuvoton Technology Corporation.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_sources_ifdef(CONFIG_ARM_MPU mpu_regions.c)
zephyr_linker_sources(SECTIONS sections.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
