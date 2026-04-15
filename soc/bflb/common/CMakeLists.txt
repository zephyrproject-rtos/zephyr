# Copyright (c) 2021-2025 ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_SERIES_BL60X OR CONFIG_SOC_SERIES_BL70X OR CONFIG_SOC_SERIES_BL70XL)
zephyr_include_directories(e24)
zephyr_sources(
e24/soc_irq_privileged.c
e24/soc.c
)
endif()

zephyr_sources(
power.c
)

# Ensure we are setup to use the required drivers
if( NOT CONFIG_CODE_DATA_RELOCATION AND (
  CONFIG_CLOCK_CONTROL
  OR CONFIG_SYSCON
  OR CONFIG_CACHE_MANAGEMENT
  OR CONFIG_FLASH) )
  message( FATAL_ERROR "Bouffalolab Platforms require usage of relocation" )
endif()

# E24 Platforms need their special CPU-tight TCMs, namely for clock control, BL61x with E907
# does not have this issue as the SRAM is coupled to CPU clock instead of bus clock
if( CONFIG_CODE_DATA_RELOCATION AND (
  CONFIG_SOC_SERIES_BL60X OR CONFIG_SOC_SERIES_BL70X OR CONFIG_SOC_SERIES_BL70XL) )
  dt_has_chosen(ITCM_EXISTS PROPERTY "zephyr,itcm")
  if( NOT ITCM_EXISTS )
    message( FATAL_ERROR "BFLB E24 Platforms requires usage of dedicated CPU TCM relocation" )
  endif()
endif()

# Relocate drivers that need to be entirely relocated
# Cache management defaults to XIP, this is fine but we need to use it in the flash driver
# Efuses do clock operations, almost all its function would need to be moved, so take the easy way
# Flash and Clock Control drivers handle relocation on their own
if( CONFIG_SOC_SERIES_BL61X )
zephyr_code_relocate(
  FILES ${ZEPHYR_BASE}/arch/riscv/custom/thead/cache_xtheadcmo.c LOCATION RAM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_SYSCON_BFLB_EFUSE LIBRARY drivers__syscon LOCATION RAM NOKEEP)
else()
zephyr_code_relocate_ifdef(CONFIG_CACHE_BFLB_L1C LIBRARY drivers__cache LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_SYSCON_BFLB_EFUSE LIBRARY drivers__syscon LOCATION ITCM NOKEEP)
endif()
