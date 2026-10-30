/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Linker script for the first stage. It is linked at the XIP window that maps
 * flash offset 0, which is where the BootROM looks for an image.
 */

ENTRY(_reset)

MEMORY
{
	FLASH (rx) : ORIGIN = 0x02000000, LENGTH = 0x1000
}

SECTIONS
{
	.text :
	{
		KEEP(*(.bk_boot_header))
		*(.text*)
		*(.rodata*)
	} > FLASH

	/DISCARD/ :
	{
		*(.ARM.exidx*)
		*(.comment*)
	}
}
