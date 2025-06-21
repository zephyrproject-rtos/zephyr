/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.ram_image2.entry,,SUBALIGN(32))
{
	__image2_entry_func__ = .;
	KEEP(*(SORT(.image2.entry.data*)))
	. = ALIGN(32);
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

SECTION_PROLOGUE(.psram_image2.text.data,,SUBALIGN(32))
{
	. = ALIGN (32);
	__ipc_table_start__ = .;
	KEEP(*(*.ipc.table.data*))
	__ipc_table_end__ = .;
} GROUP_LINK_IN(ROMABLE_REGION)
