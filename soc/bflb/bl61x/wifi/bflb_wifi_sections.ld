/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_DATA_PROLOGUE(SHAREDRAM, (NOLOAD),)
{
	. = ALIGN(4);
	__sharedram_start = .;
	*(.SHAREDRAM)
	*(SHAREDRAM)
	*(.bss.txdesc_array)
	*(.wifi_sharedram)
	. = ALIGN(4);
	__sharedram_end = .;
} GROUP_DATA_LINK_IN(WIFI_RAM, WIFI_RAM)

/* libbl616_phyrf.a build-tag -- referenced by wl_get_version; keep in ROM
 * (an explicit placement avoids an orphan-section warning at link time).
 */
SECTION_PROLOGUE(.version.phyrf,,)
{
	*(.version.phyrf)
} GROUP_ROM_LINK_IN(ROMABLE_REGION, ROMABLE_REGION)
