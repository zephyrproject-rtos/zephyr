# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(sram_config.c)
zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_SOC_RESET_HOOK mcxe31x_soc_initialization.S)

zephyr_linker_sources_ifdef(CONFIG_BOARD_NXP_MCXE31X_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

zephyr_linker_sources(SECTIONS itcm.ld)
zephyr_code_relocate(FILES sram_config.c LOCATION ${CONFIG_SRAM_CONFIG_RELOCATE_MEM}_TEXT)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
