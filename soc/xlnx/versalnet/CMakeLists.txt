#
# Copyright (c) 2025 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(
  CONFIG_ARM_MPU
  arm_mpu_regions.c
  soc.c
)
zephyr_sources_ifdef(
  CONFIG_ARM_MMU
  arm_mmu_regions.c
)

zephyr_include_directories(.)

if(CONFIG_SOC_AMD_VERSALNET_RPU)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()

if(CONFIG_SOC_AMD_VERSALNET_APU)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
