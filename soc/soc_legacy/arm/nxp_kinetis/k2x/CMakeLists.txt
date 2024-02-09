#
# Copyright (c) 2018 Prevas A/S

#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

if(DEFINED CONFIG_ARM_MPU AND DEFINED CONFIG_CPU_HAS_NXP_MPU)
  # MK22F12 series MCUs have NXP MPU
  zephyr_sources(nxp_mpu_regions.c)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
