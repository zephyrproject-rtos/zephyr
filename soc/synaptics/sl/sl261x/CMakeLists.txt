# Copyright (c) 2026 Synaptics Incorporated
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_SL2619_A55)
  zephyr_include_directories(a55)
  zephyr_sources_ifdef(CONFIG_ARM_MMU a55/mmu_regions.c)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
