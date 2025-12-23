/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
