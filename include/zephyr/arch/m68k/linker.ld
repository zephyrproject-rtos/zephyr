/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/devicetree/sram.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define RAM_BASE DT_CHOSEN_SRAM_ADDR
#define RAM_SIZE DT_CHOSEN_SRAM_SIZE
#define RAMABLE_REGION RAM

#ifdef CONFIG_XIP
#define ROM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define ROM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#define ROMABLE_REGION FLASH
#else
#define ROMABLE_REGION RAM
#endif

MEMORY
{
#ifdef CONFIG_XIP
	FLASH (rx)  : ORIGIN = ROM_BASE, LENGTH = ROM_SIZE
#endif
	RAM   (rwx) : ORIGIN = RAM_BASE, LENGTH = RAM_SIZE

	/* Build-time interrupt metadata, discarded from the final image. */
	IDT_LIST : ORIGIN = 0xfffff7ff, LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

/* Platforms without a reset shim enter the normal architecture startup. */
PROVIDE(z_m68k_reset_entry = __start);

SECTIONS
{
#ifdef CONFIG_XIP
	/* Reset fetches SSP and PC from the temporary low ROM mirror. */
	SECTION_PROLOGUE(.m68k_vector_table,,)
	{
		__m68k_vector_table_start = .;
		KEEP(*(.m68k_vector_table))
		KEEP(*(.m68k_vector_table.*))
		__m68k_vector_table_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(.m68k_boot,,)
	{
		KEEP(*(.m68k_boot))
		KEEP(*(.m68k_boot.*))
	} GROUP_LINK_IN(ROMABLE_REGION)
#else
	/* Firmware-loaded images enter the optional first-stage shim directly. */
	SECTION_PROLOGUE(.m68k_boot,,)
	{
		KEEP(*(.m68k_boot))
		KEEP(*(.m68k_boot.*))
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(.m68k_vector_table,,)
	{
		__m68k_vector_table_start = .;
		KEEP(*(.m68k_vector_table))
		KEEP(*(.m68k_vector_table.*))
		__m68k_vector_table_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
#ifndef CONFIG_XIP
		_image_ram_start = .;
#endif
		*(.text)
		*(.text.*)
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/common-rom.ld>
#include <snippets-rom-sections.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		*(.rodata)
		*(.rodata.*)
#include <snippets-rodata.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>

#ifdef CONFIG_XIP
	. = RAM_BASE;
	_image_ram_start = .;
#endif

#include <zephyr/linker/thread-local-storage.ld>

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		__data_region_start = .;
		__data_start = .;
		*(.data)
		*(.data.*)
#include <snippets-rwdata.ld>
		__data_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__data_size = __data_end - __data_start;
	__data_load_start = LOADADDR(_DATA_SECTION_NAME);

#include <zephyr/linker/common-ram.ld>
#include <snippets-ram-sections.ld>
#include <snippets-data-sections.ld>

	__data_region_end = .;
	__data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME, (NOLOAD),)
	{
		__bss_start = .;
		*(.bss)
		*(.bss.*)
		COMMON_SYMBOLS
		__bss_end = .;
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD),)
	{
		*(.noinit)
		*(.noinit.*)
#include <snippets-noinit.ld>
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/cplusplus-ram.ld>

	/*
	 * Late snippets may contribute either ROM or RAM sections.  Do not bind
	 * sections to hand-written program headers: the linker derives as many
	 * non-overlapping PT_LOAD segments as the final VMA/LMA layout requires.
	 */
#include <snippets-sections.ld>
#include <zephyr/linker/ram-end.ld>

#include <zephyr/linker/debug-sections.ld>
}
