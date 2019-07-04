/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{
    /*
     * The "locore" must be in the 64K of RAM, so that 16-bit code (with
     * segment registers == 0x0000) and 32/64-bit code agree on addresses.
     * ... there is no 16-bit code yet, but there will be when we add SMP.
     */

    .locore 0x8000 : ALIGN(16)
	{
	*(.locore)
	*(.locore.*)
	}

    /*
     * The parts of the system that don't benefit from being in the locore
     * start at the 1MB mark, otherwise the system size would be artificially
     * clamped by the ISA memory hole/ROM space at 0x90000-0xFFFFF.
     */

    .text 0x100000 : ALIGN(16)
	{
	*(.text)
	*(.text.*)
	*(.rodata.*)
	}

    /DISCARD/ :
	{
	*(.comment*)
	*(.debug*)
	*(.got*)
	*(.igot.*)
	*(.iplt)
	*(.note.GNU-stack)
	*(.rela.*)
	}
}
