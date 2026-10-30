/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#ifdef CONFIG_XIP
#define ROMABLE_REGION              ROM
#else
#define ROMABLE_REGION              RAM
#endif
#define RAMABLE_REGION              RAM

#if defined(CONFIG_ROM_END_OFFSET)
#define ROM_END_OFFSET CONFIG_ROM_END_OFFSET
#else
#define ROM_END_OFFSET 0
#endif

#ifdef CONFIG_XIP
#define ROM_BASE (DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition)))
#define ROM_SIZE (DT_REG_SIZE(DT_CHOSEN(zephyr_code_partition)) - ROM_END_OFFSET)
#else
#define ROM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define ROM_SIZE (DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) - ROM_END_OFFSET)
#endif

#define RAM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define RAM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

#include <zephyr/linker/linker-devnull.h>

MEMORY
{
#ifdef CONFIG_XIP
	ROM (rx)  : ORIGIN = ROM_BASE, LENGTH = ROM_SIZE
#endif

#if defined(CONFIG_LINKER_DEVNULL_MEMORY)
	DEVNULL_ROM (rx) : ORIGIN = DEVNULL_ADDR, LENGTH = DEVNULL_SIZE
#endif

	LINKER_DT_REGIONS()

	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST  (wx)      : ORIGIN = 0xFFFFF000, LENGTH = 4K
}

REGION_ALIAS("RAM", DT_STRING_UNQUOTED(DT_CHOSEN(zephyr_sram), zephyr_memory_region))

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
	{

#include <zephyr/linker/rel-sections.ld>
#include <zephyr/linker/llext-sections.ld>

	GROUP_START(ROMABLE_REGION)
	__rom_region_start = ROM_BASE;
	PROVIDE(___rom_region_start = __rom_region_start);

	SECTION_PROLOGUE(rom_start,,)
	{
#include <snippets-rom-start.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
		. = ALIGN(4);
		PLACE_SYMBOL_HERE(__text_region_start);

		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)
#include <zephyr/linker/kobject-text.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

	PLACE_SYMBOL_HERE(__text_region_end);

	PLACE_SYMBOL_HERE(__rodata_region_start);
#include <zephyr/linker/common-rom.ld>
#include <snippets-rom-sections.ld>
#include <zephyr/linker/thread-local-storage.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		. = ALIGN(4);
		*(.rodata)
		*(".rodata.*")
		*(.gnu.linkonce.r.*)
		*(.rosdata .rosdata.*)

#include <snippets-rodata.ld>
#include <zephyr/linker/kobject-rom.ld>
		. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>
	PLACE_SYMBOL_HERE(__rodata_region_end);

#ifndef CONFIG_XIP
	PLACE_SYMBOL_HERE(__rom_region_end);
	__rom_region_size = __rom_region_end - __rom_region_start;
	PROVIDE(___rom_region_size = __rom_region_size);
#endif
	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

	. = RAM_BASE;
	PLACE_SYMBOL_HERE(_image_ram_start);
#include <snippets-ram-sections.ld>

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		. = ALIGN(4);
		PLACE_SYMBOL_HERE(__bss_start);
		PLACE_SYMBOL_HERE(__kernel_ram_start);
		*(.sbss)
		*(".sbss.*")
		*(.bss)
		*(".bss.*")
		COMMON_SYMBOLS
		__bss_end = ALIGN(4);
		PROVIDE(___bss_end = __bss_end);
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		. = ALIGN(4);
		PLACE_SYMBOL_HERE(__data_region_start);
		PLACE_SYMBOL_HERE(__data_start);

		*(.data)
		*(".data.*")
		*(.sdata .sdata.*)

#include <snippets-rwdata.ld>

		PLACE_SYMBOL_HERE(__data_end);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	__data_size = __data_end - __data_start;
	PROVIDE(___data_size = __data_size);
	__data_load_start = LOADADDR(_DATA_SECTION_NAME);
	PROVIDE(___data_load_start = __data_load_start);

	__data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

	PROVIDE(___data_region_load_start = __data_region_load_start);

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>
#include <zephyr/linker/cplusplus-ram.ld>

#include <snippets-data-sections.ld>

	PLACE_SYMBOL_HERE(__data_region_end);

	PLACE_SYMBOL_HERE(__kernel_ram_end);
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;
	PROVIDE(___kernel_ram_size = __kernel_ram_size);

#include <snippets-sections.ld>

#define LAST_RAM_ALIGN . = ALIGN(4);

#include <zephyr/linker/ram-end.ld>

	GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

	/* RH850 ABI note, kept as in the default v850-elf-ld script */
	SECTION_PROLOGUE(.note.renesas, 0,)
	{
		KEEP(*(.note.renesas))
	}

	/* Sections generated from 'zephyr,memory-region' nodes */
	LINKER_DT_SECTIONS()

#ifdef CONFIG_XIP
	/* Must be last in romable region */
	SECTION_PROLOGUE(.last_section,,)
	{
		KEEP(*(.last_section))
		. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

	__rom_region_end = LOADADDR(.last_section) + SIZEOF(.last_section);

	PROVIDE(___rom_region_end = __rom_region_end);
	__rom_region_size = __rom_region_end - __rom_region_start;
	PROVIDE(___rom_region_size = __rom_region_size);
#endif

}
