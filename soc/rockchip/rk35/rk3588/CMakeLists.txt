# Copyright (c) 2025 Syswonder
# SPDX-License-Identifier: Apache-2.0
if(CONFIG_SOC_RK3588_AARCH64)
  zephyr_sources_ifdef(CONFIG_ARM_MMU aarch64/mmu_regions.c)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_RK3588_AARCH32)
  zephyr_sources_ifdef(CONFIG_ARM_AARCH32_MMU aarch32/mmu_regions.c)
  zephyr_sources(aarch32/soc.c)
  zephyr_include_directories(aarch32)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()
