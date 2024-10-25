/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Note: use RAM_DLM for ".ram_dlm" when a shared SRAM will be used for "zephyr,sram" */
#define TELINK_RAM_DATA_REGION         RAMABLE_REGION

SECTION_DATA_PROLOGUE(ram_dlm,,)
{
	. = ALIGN(4);
	*(.ram_dlm)
	*(".ram_dlm.*")

	PROVIDE (_RAM_DLM_VMA_END = .);
	PROVIDE (_RAM_DLM_VMA_START = ADDR(ram_dlm));
	PROVIDE (_RAM_DLM_LMA_START = LOADADDR(ram_dlm));
} GROUP_DATA_LINK_IN(TELINK_RAM_DATA_REGION, ROMABLE_REGION)
