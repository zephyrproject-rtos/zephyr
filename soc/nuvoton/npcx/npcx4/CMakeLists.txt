# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
  .
  ${ZEPHYR_BASE}/drivers
)

zephyr_sources(
  soc.c
)

zephyr_sources_ifdef(
  CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS
  mpu_regions.c
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
