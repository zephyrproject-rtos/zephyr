/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.ofsm,,)
{
	__OFSM_START = .;
	KEEP(*(.spcc))
	. = __OFSM_START + 0x8;
	KEEP(*(.tmef))
	. = __OFSM_START + 0x10;
	KEEP(*(.osis))
	. = __OFSM_START + 0x20;
	KEEP(*(.tminf))
	. = __OFSM_START + 0x24;
	KEEP(*(.mde))
	. = __OFSM_START + 0x28;
	KEEP(*(.ofs0))
	. = __OFSM_START + 0x2c;
	KEEP(*(.ofs1))
	. = __OFSM_START + 0x50;
	KEEP(*(.banksel))
	. = __OFSM_START + 0x60;
	KEEP(*(.faw))
	__OFSM_END = .;
} GROUP_LINK_IN(OFSM) = 0xFF
