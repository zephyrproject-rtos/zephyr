/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * HAL "SRAM/DRAM-only" code (flash-clock/flash-write helpers) must not run from
 * the flash XIP window; without this rule the linker orphan-places it.
 */

SECTION_PROLOGUE(.ram_image2_text,,)
{
	. = ALIGN(32);
	*(.sramdram.only.text*)
	*(.sram.only.text*)
	*(.non.dram.text*)
	*(.sramdram.only.data*)
	. = ALIGN(32);
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

/* Vendor names for the CONFIG_NOCACHE_MEMORY window, which also collects the
 * WiFi NP libraries' .nocache.data.
 */
#if defined(CONFIG_NOCACHE_MEMORY)
PROVIDE(__ram_nocache_start__ = _nocache_ram_start);
PROVIDE(__ram_nocache_end__ = _nocache_ram_end);
#endif
