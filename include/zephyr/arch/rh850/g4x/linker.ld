/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* physical address of RAM */
#ifdef CONFIG_XIP
#define ROMABLE_REGION FLASH
#else
#define ROMABLE_REGION RAM
#endif
#define RAMABLE_REGION RAM

#if defined(CONFIG_ROM_END_OFFSET)
#define ROM_END_OFFSET CONFIG_ROM_END_OFFSET
#else
#define ROM_END_OFFSET 0
#endif

/* Code partition if the board defines one, otherwise the whole flash */
#if defined(CONFIG_FLASH_USES_MAPPED_PARTITION)
#define ROM_NODE DT_CHOSEN(zephyr_code_partition)
#else
#define ROM_NODE DT_CHOSEN(zephyr_flash)
#endif

#define RAM_ADDR DT_CHOSEN_SRAM_ADDR
#define RAM_SIZE DT_CHOSEN_SRAM_SIZE

#if defined(CONFIG_XIP)
#define ROM_ADDR DT_REG_ADDR(ROM_NODE)
#define ROM_SIZE (DT_REG_SIZE(ROM_NODE) - ROM_END_OFFSET)
#else
#define ROM_ADDR RAM_ADDR
#define ROM_SIZE RAM_SIZE
#endif

_region_min_align = 4;

#include <zephyr/linker/linker-devnull.h>

MEMORY
{
	FLASH (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
	RAM   (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
#if defined(CONFIG_LINKER_DEVNULL_MEMORY)
	DEVNULL_ROM (rx) : ORIGIN = DEVNULL_ADDR, LENGTH = DEVNULL_SIZE
#endif
	LINKER_DT_REGIONS()
	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST (wx) : ORIGIN = (RAM_ADDR + RAM_SIZE), LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

	GROUP_START(ROMABLE_REGION)

	__rom_region_start = ROM_ADDR;
	PROVIDE(_CONCAT(_, __rom_region_start) = __rom_region_start);

	SECTION_PROLOGUE(rom_start,,)
	{
		PLACE_SYMBOL_HERE(_vector_start);
		KEEP(*(.exc_vector_table))
		KEEP(*(".exc_vector_table.*"))

#if LINKER_ZEPHYR_FINAL && defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
		INCLUDE isr_tables_vt.ld
#else
		KEEP(*(.vectors))
#endif
		PLACE_SYMBOL_HERE(_vector_end);

/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_START ...). This typically contains the vector
 * table and debug information.
 */
#include <snippets-rom-start.ld>

	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_CODE_DATA_RELOCATION

#include <linker_relocate.ld>

#endif /* CONFIG_CODE_DATA_RELOCATION */

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
		PLACE_SYMBOL_HERE(__text_region_start);

#include <zephyr/linker/kobject-text.ld>

		*(.text)
		*(".text.*")
		*(".TEXT.*")
		*(.gnu.linkonce.t.*)
		. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

	PLACE_SYMBOL_HERE(__text_region_end);

	PLACE_SYMBOL_HERE(__rodata_region_start);

#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
#include <zephyr/linker/thread-local-storage.ld>
	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		*(.rodata)
		*(".rodata.*")
		*(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <zephyr/linker/kobject-rom.ld>

		. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>

	PLACE_SYMBOL_HERE(__rodata_region_end);
	. = ALIGN(_region_min_align);

	__rom_region_end = __rom_region_start + . - ADDR(rom_start);
	PROVIDE(_CONCAT(_, __rom_region_end) = __rom_region_end);
	__rom_region_size = __rom_region_end - __rom_region_start;
	PROVIDE(_CONCAT(_, __rom_region_size) = __rom_region_size);

	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

#if ROM_ADDR != RAM_ADDR
	. = RAM_ADDR;
#endif

	/* Align the start of image RAM with the
	 * minimum granularity required by MPU.
	 */
	. = ALIGN(_region_min_align);
	PLACE_SYMBOL_HERE(_image_ram_start);

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

	GROUP_START(DATA_REGION)

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		PLACE_SYMBOL_HERE(__data_region_start);
		PLACE_SYMBOL_HERE(__data_start);
		*(.data)
		*(".data.*")
		*(".kernel.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_data_relocate.ld>
#endif
		PLACE_SYMBOL_HERE(__data_end);

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	__data_size = __data_end - __data_start;
	PROVIDE(_CONCAT(_, __data_size) = __data_size);
	__data_load_start = LOADADDR(_DATA_SECTION_NAME);
	PROVIDE(_CONCAT(_, __data_load_start) = __data_load_start);

	__data_region_load_start = LOADADDR(_DATA_SECTION_NAME);
	PROVIDE(_CONCAT(_, __data_region_load_start) = __data_region_load_start);

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>

#include <zephyr/linker/cplusplus-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>
	PLACE_SYMBOL_HERE(__data_region_end);

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#include <zephyr/linker/debug-sections.ld>

	/DISCARD/ : { *(.note.GNU-stack) }
	/DISCARD/ : { *(.note.renesas) }

/* Sections generated from 'zephyr,memory-region' nodes */
	LINKER_DT_SECTIONS()

/* Must be last in romable region */
	SECTION_PROLOGUE(.last_section,,)
	{
		/*
		 * .last_section contains a fixed word to ensure location counter and actual
		 * rom region data usage match when CONFIG_LINKER_LAST_SECTION_ID=y.
		 */
		KEEP(*(.last_section))
	} GROUP_LINK_IN(ROMABLE_REGION)

/* To provide the image size as a const expression,
 * calculate this value here. */
	_flash_used = LOADADDR(.last_section) + SIZEOF(.last_section) - __rom_region_start;
	PROVIDE(_CONCAT(_, _flash_used) = _flash_used);

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		. = ALIGN(4);
		PLACE_SYMBOL_HERE(__bss_start);
		PLACE_SYMBOL_HERE(__kernel_ram_start);

#include <snippets-bss.ld>

		*(.bss)
		*(".bss.*")
		*(COMMON)
		*(".kernel_bss.*")

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_bss_relocate.ld>
#endif

		/*
		 * As memory is cleared in words only, it is simpler to ensure the BSS
		 * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		 */
		. = ALIGN(4);
		PLACE_SYMBOL_HERE(__bss_end);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

	/* Define linker symbols */

	__kernel_ram_end = RAM_ADDR + RAM_SIZE;
	PROVIDE(_CONCAT(_, __kernel_ram_end) = __kernel_ram_end);
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;
	PROVIDE(_CONCAT(_, __kernel_ram_size) = __kernel_ram_size);

	SECTION_PROLOGUE(.last_ram_section, (NOLOAD),)
	{
#ifdef LAST_RAM_ALIGN
		LAST_RAM_ALIGN
#endif
		PLACE_SYMBOL_HERE(_image_ram_end);
		_image_ram_size = _image_ram_end - _image_ram_start;
		PROVIDE(_CONCAT(_, _image_ram_size) = _image_ram_size);
		PLACE_SYMBOL_HERE(_end); /* end of image */
		PLACE_SYMBOL_HERE(z_mapped_end);
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	GROUP_END(RAMABLE_REGION)

}
