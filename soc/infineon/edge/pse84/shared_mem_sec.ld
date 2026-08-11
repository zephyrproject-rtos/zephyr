/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

GROUP_START(M33SHARED)

	SECTION_PROLOGUE(.cy_sharedmem,(NOLOAD),)
	{
		. = ALIGN(4);
		__public_ram_start__ = .;
		KEEP(*(.cy_sharedmem))
		. = ALIGN(4);
		__public_ram_end__ = .;
	} GROUP_LINK_IN(M33SHARED)

GROUP_END(M33SHARED)

GROUP_START(SECSHARED)

	SECTION_PROLOGUE(.cy_sharedmem_sec,(NOLOAD),)
	{
        . = ALIGN(4);
        __sec_ram_start__ = .;
        KEEP(*(.cy_sharedmem_sec))
        . = ALIGN(4);
        __sec_ram_end__ = .;
	} GROUP_LINK_IN(SECSHARED)

GROUP_END(SECSHARED)
