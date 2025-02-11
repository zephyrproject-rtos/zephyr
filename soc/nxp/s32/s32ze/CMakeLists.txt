# Copyright 2022,2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(
  soc.c
)

zephyr_library_sources_ifdef(CONFIG_ARM_MPU mpu_regions.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
