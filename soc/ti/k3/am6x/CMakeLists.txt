# Copyright (c) 2023 Enphase Energy
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_AM6234_A53)
  zephyr_sources_ifdef(CONFIG_ARM_MMU a53/mmu_regions.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_AM6234_M4)
  zephyr_sources(m4/soc.c)

  zephyr_include_directories(m4)

  if(CONFIG_OPENAMP_RSC_TABLE)
    zephyr_linker_section(NAME .resource_table GROUP ROM_REGION NOINPUT)
    zephyr_linker_section_configure(SECTION .resource_table KEEP INPUT ".resource_table*")
  endif()

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/m4/linker.ld CACHE INTERNAL "")
endif()
