# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  )
zephyr_sources_ifdef(
  CONFIG_ARM_MPU
  nxp_mpu_regions.c
  )

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
