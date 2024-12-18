/*
 * Copyright (c) 2024 - 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_DATA_PROLOGUE(.dtc_vector_table,(NOLOAD),)
{
	/* If DTC is used, put the DTC vector table at the start of SRAM.
		This avoids memory holes due to 1K alignment required by it. */
	*(.dtc_vector_table)
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

SECTION_PROLOGUE(.ofsm,,)
{
	__OFSM_START = .;
	KEEP(*(.ofs_mde))
	. = __OFSM_START + 0x8;
	KEEP(*(.ofs1))
	. = __OFSM_START + 0xC;
	KEEP(*(.ofs0))
	__OFSM_END = .;
} GROUP_LINK_IN(OFSM) = 0xFF
