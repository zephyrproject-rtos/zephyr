# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_T153_E907)
  set(SOC_LINKER_SCRIPT
    "${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld"
    CACHE INTERNAL ""
  )

  zephyr_sources(
    irq_context.S
    soc.c
  )

  zephyr_linker_sources_ifdef(CONFIG_OPENAMP_RSC_TABLE ROM_SECTIONS
    resource_table.ld
  )
elseif(CONFIG_SOC_T153_A7)
  zephyr_include_directories(a7)

  zephyr_sources(a7/soc.c)
  zephyr_sources_ifdef(CONFIG_ARM_AARCH32_MMU a7/mmu_regions.c)

  set(SOC_LINKER_SCRIPT
    "${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld"
    CACHE INTERNAL ""
  )
endif()
