# Copyright (c) 2026 Jiaming Zhao
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_RK3506_A7)
  zephyr_include_directories(.)
  zephyr_library_sources(soc.c smccc.c)
  zephyr_library_sources_ifdef(CONFIG_ARM_AARCH32_MMU mmu_regions.c)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()
