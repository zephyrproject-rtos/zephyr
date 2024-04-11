/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TELINK_RAM_CODE_REGION         RAM_ILM

SECTION_DATA_PROLOGUE(ram_code,,)
{
	. = ALIGN(4);
	*(.ram_code)
	*(".ram_code.*")

	PROVIDE (_RAM_CODE_VMA_END = .);
	PROVIDE (_RAM_CODE_VMA_START = ADDR(ram_code));
	PROVIDE (_RAM_CODE_LMA_START = LOADADDR(ram_code));
} GROUP_DATA_LINK_IN(TELINK_RAM_CODE_REGION, ROMABLE_REGION)
