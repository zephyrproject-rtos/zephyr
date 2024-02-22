# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX8ML8_A53 OR CONFIG_SOC_MIMX8MM6_A53 OR CONFIG_SOC_MIMX8MN6_A53)
  zephyr_include_directories(.)
  zephyr_include_directories(a53)

  zephyr_sources_ifdef(CONFIG_ARM_MMU a53/mmu_regions.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
