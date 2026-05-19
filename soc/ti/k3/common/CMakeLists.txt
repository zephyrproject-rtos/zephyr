# Copyright (c) 2023 Enphase Energy
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(peripherals)
zephyr_sources(peripherals/ctrl_partitions.c)

if(CONFIG_SOC_SERIES_AM62X_A53 OR CONFIG_SOC_SERIES_AM62LX_A53)
  zephyr_sources_ifdef(CONFIG_ARM_MMU cortex_a/mmu_regions.c)
  zephyr_sources(cortex_a/soc.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_SERIES_AM62X_M4 OR CONFIG_SOC_SERIES_AM64X_M4)
  zephyr_sources(cortex_m/soc.c)

  zephyr_include_directories(cortex_m)

  if(CONFIG_OPENAMP_RSC_TABLE)
    zephyr_linker_section(NAME .resource_table GROUP ROM_REGION NOINPUT)
    zephyr_linker_section_configure(SECTION .resource_table KEEP INPUT ".resource_table*")
  endif()

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cortex_m/linker.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_SERIES_AM64X_R5 OR CONFIG_SOC_SERIES_J721E_R5 OR CONFIG_SOC_SERIES_J722S_R5)
  zephyr_sources(cortex_r/soc.c)
  zephyr_sources_ifdef(CONFIG_ARM_MPU cortex_r/arm_mpu_regions.c)

  zephyr_include_directories(cortex_r)

  if(CONFIG_OPENAMP_RSC_TABLE)
    zephyr_linker_section(NAME .resource_table GROUP ROM_REGION NOINPUT)
    zephyr_linker_section_configure(SECTION .resource_table KEEP INPUT ".resource_table*")
  endif()

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cortex_r/linker.ld CACHE INTERNAL "")
endif()
