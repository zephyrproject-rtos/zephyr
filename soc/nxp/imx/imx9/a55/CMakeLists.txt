# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)
set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

if(CONFIG_SOC_MIMX9596_A55)
  zephyr_sources(soc.c)
endif()
