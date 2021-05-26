/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
MEMORY
{
	BOOT_FLASH (r) : ORIGIN = 0x10000000, LENGTH = 256
}

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>

SECTIONS
{
	SECTION_PROLOGUE(BOOT_FLASH,, SUBALIGN(4))
	{
		KEEP(*(.boot2))
	}
}
