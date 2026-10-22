/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

GROUP_START(EXTRAM)

SECTION_DATA_PROLOGUE(EXTRAM_DATA,,)
{
	. = ALIGN(4);
	__extram_data_start = .;
	*(.ext.datasram.data)
	__extram_data_end = ALIGN(4);
} GROUP_DATA_LINK_IN(EXTRAM, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(EXTRAM_BSS,(NOLOAD),)
{
	. = ALIGN(4);
	__extram_bss_start = .;
	*(.ext.datasram.bss)
	__extram_bss_end = ALIGN(4);
} GROUP_DATA_LINK_IN(EXTRAM, EXTRAM)

__extram_data_load_start = LOADADDR(EXTRAM_DATA);

GROUP_END(EXTRAM)
