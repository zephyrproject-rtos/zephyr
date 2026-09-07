/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
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

#define _EXCEPTION_SECTION_NAME     exceptions
#define _RESET_SECTION_NAME         reset

#if defined(CONFIG_ROM_END_OFFSET)
#define ROM_END_OFFSET CONFIG_ROM_END_OFFSET
#else
#define ROM_END_OFFSET 0
#endif

#ifdef CONFIG_XIP
#define ROM_BASE (DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition)))
#define ROM_SIZE (DT_REG_SIZE(DT_CHOSEN(zephyr_code_partition)) - ROM_END_OFFSET)
#else /* CONFIG_XIP */
#define ROM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define ROM_SIZE (DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) - ROM_END_OFFSET)
#endif /* CONFIG_XIP */

#define RAM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define RAM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

/* Alignment hook emitted by the code and data relocation generator */
#define MPU_ALIGN(region_size) . = ALIGN(8)

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

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_itcm), okay)
	REGION_ALIAS("ITCM", DT_STRING_UNQUOTED(DT_CHOSEN(zephyr_itcm), zephyr_memory_region))
#endif

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
	REGION_ALIAS("DTCM", DT_STRING_UNQUOTED(DT_CHOSEN(zephyr_dtcm), zephyr_memory_region))
#endif

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
	{

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

	/* .plt/.iplt precede .text per 'tricore-elf-ld --verbose'; discard them. */
	/DISCARD/ :
	{
	*(.plt)
	}

	/DISCARD/ :
	{
	*(.iplt)
	}

	GROUP_START(ROMABLE_REGION)
	__rom_region_start = ROM_BASE;

	SECTION_PROLOGUE(rom_start,,)
	{
		. = ALIGN(32);
#include <snippets-rom-start.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_relocate.ld>
#endif

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
		. = ALIGN(4);
		KEEP(*(.openocd_debug))
		KEEP(*(".openocd_debug.*"))

		__text_region_start = .;

		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)
#include <zephyr/linker/kobject-text.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

	/* Place Trap and Interrupt vector tables at the end of the text segment */
	SECTION_PROLOGUE(.vectors.trap,,)
	{
		. = ALIGN(256);
		__btv = .;
		KEEP(*(.vectors.trap*))
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(.vectors.irq,,)
	{
		. = ALIGN(8192);
		__biv = .;
		KEEP(*(SORT_BY_NAME(.vectors.irq*)))
	} GROUP_LINK_IN(ROMABLE_REGION)

	__text_region_end = .;

	__rodata_region_start = .;
#include <zephyr/linker/common-rom.ld>
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
		 *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)

#include <snippets-rodata.ld>
#include <zephyr/linker/kobject-rom.ld>
	. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>
	__rodata_region_end = .;

#ifndef CONFIG_XIP
	__rom_region_end = .;
	__rom_region_size = __rom_region_end - __rom_region_start;
#endif /* CONFIG_XIP */
	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

	. = RAM_BASE;
	_image_ram_start = .;
#include <snippets-ram-sections.ld>

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		. = ALIGN(8);
		/* BSS is cleared word-wise; keep it 4-byte aligned. */
		 . = ALIGN(4);
		 __bss_start = .;
		__kernel_ram_start = .;
		 *(.sbss)
		 *(".sbss.*")
		 *(.bss)
		 *(".bss.*")
		 COMMON_SYMBOLS

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_bss_relocate.ld>
#endif

		  __bss_end = ALIGN(4);
	}  GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		. = ALIGN(4);
		__data_region_start = .;
		__data_start = .;

		*(.data)
		*(".data.*")

		*(.sdata .sdata.* .gnu.linkonce.s.*)

#include <snippets-rwdata.ld>

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_data_relocate.ld>
#endif

		__data_end = .;

	}  GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	__data_size = __data_end - __data_start;
	__data_load_start = LOADADDR(_DATA_SECTION_NAME);

	__data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>
#include <zephyr/linker/cplusplus-ram.ld>

#include <snippets-data-sections.ld>

	__data_region_end = .;

	__kernel_ram_end = .;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_itcm), okay)
GROUP_START(ITCM)

	SECTION_PROLOGUE(_ITCM_SECTION_NAME,,SUBALIGN(8))
	{
		__itcm_start = .;
		*(.itcm)
		*(".itcm.*")

#include <snippets-itcm-section.ld>

		__itcm_end = .;
	} GROUP_LINK_IN(ITCM AT> ROMABLE_REGION)

	__itcm_size = __itcm_end - __itcm_start;
	__itcm_load_start = LOADADDR(_ITCM_SECTION_NAME);

GROUP_END(ITCM)
#endif

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
GROUP_START(DTCM)

	SECTION_PROLOGUE(_DTCM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(8))
	{
		__dtcm_start = .;
		__dtcm_bss_start = .;
		*(.dtcm_bss)
		*(".dtcm_bss.*")
		__dtcm_bss_end = .;
	} GROUP_LINK_IN(DTCM)

	SECTION_PROLOGUE(_DTCM_NOINIT_SECTION_NAME, (NOLOAD),SUBALIGN(8))
	{
		__dtcm_noinit_start = .;
		*(.dtcm_noinit)
		*(".dtcm_noinit.*")
		__dtcm_noinit_end = .;
	} GROUP_LINK_IN(DTCM)

	SECTION_PROLOGUE(_DTCM_DATA_SECTION_NAME,,SUBALIGN(8))
	{
		__dtcm_data_start = .;
		*(.dtcm_data)
		*(".dtcm_data.*")

#include <snippets-dtcm-section.ld>

		__dtcm_data_end = .;
	} GROUP_LINK_IN(DTCM AT> ROMABLE_REGION)

	__dtcm_end = .;

	__dtcm_data_load_start = LOADADDR(_DTCM_DATA_SECTION_NAME);

GROUP_END(DTCM)
#endif

#include <snippets-sections.ld>

#define LAST_RAM_ALIGN . = ALIGN(8);

#include <zephyr/linker/ram-end.ld>

	GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

	/DISCARD/ : { *(.callinfo) }
	/DISCARD/ : { *(.note.GNU-stack) }

	/* Output sections silence lld's "--orphan-handling=warn". */
#if defined(CONFIG_LLVM_USE_LLD)
	SECTION_PROLOGUE(.symtab, 0,) { *(.symtab) }
	SECTION_PROLOGUE(.strtab, 0,) { *(.strtab) }
	SECTION_PROLOGUE(.shstrtab, 0,) { *(.shstrtab) }
#endif

	/* lld keeps .eh_frame even with --gc-sections; discard it. */
#if !defined(CONFIG_CPP_EXCEPTIONS)
	/DISCARD/ : { *(.eh_frame) }
#endif

	/* Sections generated from 'zephyr,memory-region' nodes */
	LINKER_DT_SECTIONS()

#ifdef CONFIG_XIP
	/* Must be last in romable region */
SECTION_PROLOGUE(.last_section,,)
{
  KEEP(*(.last_section))
  . = ALIGN(8);
} GROUP_LINK_IN(ROMABLE_REGION)

__rom_region_end = LOADADDR(.last_section) + SIZEOF(.last_section);
__rom_region_size = __rom_region_end - __rom_region_start;
#endif

}
