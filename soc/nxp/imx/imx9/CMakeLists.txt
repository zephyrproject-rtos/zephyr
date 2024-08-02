# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX9352_A55)
  zephyr_include_directories(.)
  zephyr_include_directories(a55)

  zephyr_sources_ifdef(CONFIG_ARM_MMU a55/mmu_regions.c)

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/a55/linker.ld CACHE INTERNAL "")
endif()
