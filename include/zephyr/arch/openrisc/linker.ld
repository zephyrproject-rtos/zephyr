/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Generic Linker script for the OpenRISC platform
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* Physical address of RAM */
#ifdef CONFIG_XIP
#define ROMABLE_REGION FLASH
#else
#define ROMABLE_REGION RAM
#endif
#define RAMABLE_REGION RAM

#if !defined(CONFIG_XIP) && (CONFIG_FLASH_SIZE == 0)
#define ROM_BASE RAM_BASE
#else
#define ROM_BASE CONFIG_FLASH_BASE_ADDRESS
#endif

#if CONFIG_FLASH_LOAD_SIZE > 0
#define ROM_SIZE (CONFIG_FLASH_LOAD_SIZE)
#else
#define ROM_SIZE (CONFIG_FLASH_SIZE * 1024)
#endif

#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K)
#define RAM_BASE CONFIG_SRAM_BASE_ADDRESS

#define _EXCEPTION_SECTION_NAME	exception

#define MPU_MIN_SIZE_ALIGN
#define MPU_ALIGN(region_size) . = ALIGN(4)

MEMORY
{
    FLASH (rx) : ORIGIN = ROM_BASE, LENGTH = ROM_SIZE
    RAM  (rwx) : ORIGIN = RAM_BASE, LENGTH = RAM_SIZE
    LINKER_DT_REGIONS()
    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

PROVIDE (__memory_base = CONFIG_SRAM_BASE_ADDRESS);
PROVIDE (__memory_size = CONFIG_SRAM_SIZE * 1024);
PROVIDE (__stack = CONFIG_SRAM_BASE_ADDRESS + (CONFIG_SRAM_SIZE - 1) * 1024);

SECTIONS
{
#include <zephyr/linker/rel-sections.ld>

    SECTION_PROLOGUE(.plt,,)
	{
		*(.plt)
	}

    SECTION_PROLOGUE(.iplt,,)
	{
		*(.iplt)
	}

    __text_region_start = ROM_BASE;

    SECTION_PROLOGUE(_EXCEPTION_SECTION_NAME,,)
	{
		KEEP(*(.exceptions))
		KEEP(*(.exceptions.*))
	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_relocate.ld>
#endif

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
		. = ALIGN(4);

		*(.text)
		*(".text.*")
	} GROUP_LINK_IN(ROMABLE_REGION)

    __text_region_end = .;

    __rodata_region_start = .;
#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
#include <zephyr/linker/thread-local-storage.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		 . = ALIGN(4);
		 *(.srodata)
		 *(".srodata.*")
		 *(.rodata)
		 *(".rodata.*")
		 *(.gnu.linkonce.r.*)
		 *(.rodata1)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>
#include <zephyr/linker/kobject-rom.ld>
		. = ALIGN(4);

	} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>
    __rodata_region_end = .;

	. = RAM_BASE;
	_image_ram_start = .;
/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		 . = ALIGN(4);
		 __data_region_start = .;
		 __data_start = .;

		 *(.data)
		 *(.data.*)
		 *(.gnu.linkonce.d.*)
		 *(.sdata)
		 *(.sdata.*)
		 . = ALIGN(8);
		SORT(CONSTRUCTORS)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_data_relocate.ld>
#endif

		__data_end = .;

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	__data_size = __data_end - __data_start;
	__data_load_start = LOADADDR(_DATA_SECTION_NAME);

	__data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>
#include <zephyr/linker/cplusplus-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

    __data_region_end = .;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		 . = ALIGN(4);
		 __bss_start = .;
		 *(.dynbss)
		 *(.sbss)
		 *(.sbss.*)
		 *(.bss)
		 *(.bss.*)
		 *(.gnu.linkonce.b.*)
		 *(.scommon)
		 COMMON_SYMBOLS

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_bss_relocate.ld>
#endif

		 /*
		  * As memory is cleared in words only, it is simpler to ensure the BSS
		  * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		  */
		 __bss_end = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

#include <zephyr/linker/ram-end.ld>

    GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

	.mdebug.abi32 : {
		KEEP(*(.mdebug.abi32))
	}

    SECTION_PROLOGUE(.gnu.attributes, 0,)
	{
		KEEP(*(.gnu.attributes))
	}

	/DISCARD/ : {
		*(.pdr)
		*(.reginfo)
	}

    /* Sections generated from 'zephyr,memory-region' nodes */
    LINKER_DT_SECTIONS()
}
