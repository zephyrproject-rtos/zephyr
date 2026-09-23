# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MCXE32B_CPU1)
  zephyr_compile_definitions(CPU_MCXE32BMPB_cm7_core1)
else()
  zephyr_compile_definitions(CPU_MCXE32BMPB_cm7_core0)
endif()

zephyr_sources(soc.c)
zephyr_sources(sram_config.c)
zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)
zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_SOC_EARLY_RESET_HOOK mcxe32x_soc_initialization.S)

zephyr_linker_sources_ifdef(CONFIG_BOARD_NXP_MCXE32X_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

zephyr_linker_sources(SECTIONS itcm.ld)
zephyr_code_relocate(FILES sram_config.c LOCATION ${CONFIG_SRAM_CONFIG_RELOCATE_MEM}_TEXT)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
