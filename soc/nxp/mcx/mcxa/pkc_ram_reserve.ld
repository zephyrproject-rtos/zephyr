/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Reserve the low 16 KB of SRAM (SRAM A0/A1/A2, RAM_ADDR..RAM_ADDR+0x3FFF)
 * for the SGI PKC engine's CPU/PKC workareas, mirroring the MCUXpresso SDK's
 * __pkc__ linker carve-out (__ram_offset__ = 0x4000). Only pulled in via
 * zephyr_linker_sources_ifdef(CONFIG_SGI_PKC ...), so sram0's devicetree
 * size never changes; builds without CONFIG_SGI_PKC get the full region.
 */
SECTION_DATA_PROLOGUE(pkc_ram_reserve,(NOLOAD),)
{
	. += 0x4000;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
