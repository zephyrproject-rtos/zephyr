/*
* Copyright (c) 2018 ispace, inc.
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file
* @brief Linker command/script file
*
* Linker script for the LEON2 platform.
*/

#include <autoconf.h>
#include <linker/sections.h>
#include <devicetree.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

/* physical address of RAM */
#define ROMABLE_REGION SRAM
#define RAMABLE_REGION SRAM

/* TBR tt and zero field (12 bit) */
#define VECTOR_TABLE_SIZE 0x1000

MEMORY
{
	SRAM                  (wx) : ORIGIN = 0x40000000, LENGTH = MB(1)
	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{
	#include <linker/rel-sections.ld>

	SECTION_PROLOGUE(vector,,)
	{
		_vector_start = .;
		*(.vector_table)
		*(".vector_table.*")
		_vector_end = VECTOR_TABLE_SIZE;

	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
		. = ALIGN(4);

		_image_text_start = .;
		*(.text*)
		*(.gnu.linkonce.t.*)
		_image_text_end = .;

	} GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/common-rom.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		. = ALIGN(4);
		_image_rodata_start = .;

		*(.rodata)
		*(".rodata.*")
		*(.gnu.linkonce.r.*)

		. = ALIGN(4);
		_image_rodata_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

       /*
	* .plt and .iplt are here according to
     	* 'sparc-zephyr-elf-ld --verbose', before text section.
	*/
	SECTION_PROLOGUE(.plt,,)
        {
        *(.plt)
        }

	SECTION_PROLOGUE(.iplt,,)
	{
	*(.iplt)
	}

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		. = ALIGN(4);

		*(.data)
		*(".data.*")

		. = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <linker/common-ram.ld>

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		. = ALIGN(4);
		__bss_start = .;

		*(.bss)
		*(".bss.*")
		*(COMMON)

		. = ALIGN(4);
		__bss_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
	{
		. = ALIGN(4);

		*(.noinit)
		*(".noinit.*")

		. = ALIGN(4);
		_ram_region_end = ORIGIN(SRAM) + LENGTH(SRAM);
		_image_ram_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif

#include <linker/debug-sections.ld>

	SECTION_PROLOGUE(.sparc.attributes, 0,)
	{
		KEEP(*(.gnu.attributes))
	}
}
