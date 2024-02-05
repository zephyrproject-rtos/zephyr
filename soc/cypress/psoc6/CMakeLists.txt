#
# Copyright (c) 2018, Cypress
# Copyright (c) 2020, ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_SERIES_PSOC62 OR CONFIG_SOC_SERIES_PSOC63)
  add_subdirectory(old/common)
  zephyr_include_directories(old)
  zephyr_sources(old/soc.c)

  zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_PSOC6 NOINIT old/noinit.ld)
  zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_PSOC6 RWDATA old/rwdata.ld)
else()
  zephyr_include_directories(new)
  zephyr_include_directories(new/common)
  zephyr_sources(new/soc.c)

  # Add sections
  zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 NOINIT new/noinit.ld)

  # Add section for cm0p image ROM
  zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1A ROM_START SORT_KEY 0x0cm0p new/rom_cm0image.ld)

  # Add section for cm0p image RAM
  zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1A RAM_SECTIONS SORT_KEY 0 new/ram_cm0image.ld)
  zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1A RAMFUNC_SECTION SORT_KEY 0 new/ram_func.ld)
  zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 RODATA SORT_KEY 0 new/rom.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
