# Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(soc.c)

zephyr_linker_sources(SECTIONS sections.ld)
zephyr_linker_sources(DATA_SECTIONS data_sections.ld)
zephyr_linker_sources(RAM_SECTIONS ram_sections.ld)
zephyr_linker_sources(ROM_START rom_start.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
