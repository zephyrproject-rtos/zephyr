# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_PSOC6)
  zephyr_include_directories(common)
  zephyr_sources(common/soc.c)

  # Add sections
  zephyr_linker_sources(NOINIT common/noinit.ld)

  # Add section for cm0p image ROM
  zephyr_linker_sources(ROM_START SORT_KEY 0x0cm0p common/rom_cm0image.ld)

  # Add section for cm0p image RAM
  zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0 common/ram_cm0image.ld)
  zephyr_linker_sources(RAMFUNC_SECTION SORT_KEY 0 common/ram_func.ld)
  zephyr_linker_sources(RODATA SORT_KEY 0 common/rom.ld)

endif()

if(CONFIG_SOC_FAMILY_PSOC6_LEGACY)
  zephyr_include_directories(psoc6_legacy)
  zephyr_sources(psoc6_legacy/soc.c)

  zephyr_linker_sources(NOINIT psoc6_legacy/noinit.ld)
  zephyr_linker_sources(RWDATA psoc6_legacy/rwdata.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
