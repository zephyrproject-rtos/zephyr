/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define TELINK_RAM_DATA_REGION         RAM_DLM

/*
 * Moving BLE library sections (without suffix) to RAM DLM (non-retention)
 */
SECTION_DATA_PROLOGUE(ram_dlm,,)
{
	. = ALIGN(4);
	*(.data)
	*(.sdata)
	*(.bss)

	PROVIDE (_RAM_DLM_VMA_END = .);
	PROVIDE (_RAM_DLM_VMA_START = ADDR(ram_dlm));
	PROVIDE (_RAM_DLM_LMA_START = LOADADDR(ram_dlm));
} GROUP_DATA_LINK_IN(TELINK_RAM_DATA_REGION, ROMABLE_REGION)
