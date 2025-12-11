# Copyright (c) 2023 IoT.bzh
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_R8A779F0_R52)
  zephyr_include_directories(r52)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_R8A779F0_A55)
  zephyr_include_directories(a55)
  zephyr_library_sources_ifdef(CONFIG_ARM_MMU a55/mmu_regions.c)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
