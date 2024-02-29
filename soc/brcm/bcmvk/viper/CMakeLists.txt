# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_BCM58402_A72)
  zephyr_include_directories(a72)

  zephyr_sources(
    a72/soc.c
    a72/plat_core.c
  )

  zephyr_sources_ifdef(CONFIG_ARM_MMU a72/mmu_regions.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_BCM58402_M7)
  zephyr_include_directories(m7)
  zephyr_sources(m7/soc.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
endif()
