#
# Copyright (c) 2025 Space Cubics Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(CONFIG_SOC_VERSAL_RPU soc.c)
zephyr_sources_ifdef(CONFIG_ARM_MMU arm_mmu_regions.c)

zephyr_include_directories(.)

if(CONFIG_SOC_VERSAL_RPU)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()

if(CONFIG_SOC_VERSAL_APU)
  # Use generic ARM64 linker script
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
