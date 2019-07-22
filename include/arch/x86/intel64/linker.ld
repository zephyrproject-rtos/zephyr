/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#define _LINKER
#define _ASMLANGUAGE

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define ROMABLE_REGION RAM
#define RAMABLE_REGION RAM

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
	 * The rest of the system is loaded in "normal" memory (typically
	 * placed above 1MB to avoid the by memory hole at 0x90000-0xFFFFF).
	 */

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,ALIGN(16))
	{
	*(.text)
	*(.text.*)
	} GROUP_LINK_IN(ROMABLE_REGION)

	#include <linker/common-rom.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,ALIGN(16))
	{
	_image_rodata_start = .;

	*(.rodata)
	*(.rodata.*)

	#include <snippets-rodata.ld>

	#ifdef CONFIG_CUSTOM_RODATA_LD
	#include <custom-rodata.ld>
	#endif /* CONFIG_CUSTOM_RODATA_LD */

	_image_rodata_end = .;
	_image_rodata_size = _image_rodata_end - _image_rodata_start;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_DATA_SECTION_NAME,,ALIGN(16))
	{
	*(.data)
	*(.data.*)
	#include <snippets-rwdata.ld>
	#ifdef CONFIG_CUSTOM_RWDATA_LD
	#include <custom-rwdata.ld>
	#endif /* CONFIG_CUSTOM_RWDATA_LD */
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <snippets-ram-sections.ld>
#include <linker/common-ram.ld>

	SECTION_PROLOGUE(_BSS_SECTION_NAME, (NOLOAD), ALIGN(16))
	{
	__bss_start = .;
	*(.bss)
	*(.bss.*)
	*(COMMON)
	. = ALIGN(8);	/* so __bss_num_qwords is exact */
	__bss_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	__bss_num_qwords = (__bss_end - __bss_start) >> 3;

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD), ALIGN(16))
	{
	*(.noinit)
	*(.noinit.*)
	#include <snippets-noinit.ld>
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <snippets-sections.ld>
#ifdef CONFIG_CUSTOM_SECTIONS_LD
#include <custom-sections.ld>
#endif /* CONFIG_CUSTOM_SECTIONS_LD */

	. = ALIGN(8);
	_end = .;

	/DISCARD/ :
	{
	*(.comment*)
	*(.debug*)
	*(.got)
	*(.got.plt)
	*(.igot)
	*(.igot.plt)
	*(.iplt)
	*(.plt)
	*(.note.GNU-stack)
	*(.rel.*)
	*(.rela.*)
	}
}
