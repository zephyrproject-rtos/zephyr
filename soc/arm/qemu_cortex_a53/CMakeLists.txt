# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_QEMU_CORTEX_A53_EXECUTION_STATE_AARCH32)
  zephyr_library_sources_ifdef(CONFIG_ARM_AARCH32_MMU aarch32/mmu_regions.c)
  zephyr_include_directories(aarch32)

  set(SOC_LINKER_SCRIPT
      ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld
      CACHE INTERNAL ""
  )
elseif(CONFIG_QEMU_CORTEX_A53_EXECUTION_STATE_AARCH64)
  zephyr_include_directories(.)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
