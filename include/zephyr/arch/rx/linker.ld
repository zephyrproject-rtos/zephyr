/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Generic Linker script for the riscv platform
 */

#include <zephyr/devicetree.h>

#include <zephyr/linker/sections.h>
#include <zephyr/linker/devicetree_regions.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#ifdef CONFIG_XIP
#define ROMABLE_REGION	ROM
#else
#define ROMABLE_REGION	RAM
#endif
#define RAMABLE_REGION	RAM

/* single bank configuration with 2 MB of code flash */
#define ROM_START (DT_REG_ADDR(DT_NODELABEL(code_flash)))

#define ROM_SIZE  (DT_REG_SIZE(DT_NODELABEL(code_flash)))

#define RAM_START (CONFIG_SRAM_BASE_ADDRESS)
#define RAM_SIZE  (KB(CONFIG_SRAM_SIZE))

_region_min_align = 4;

MEMORY
{
	ROM (rx) : ORIGIN = ROM_START, LENGTH = ROM_SIZE
	RAM (rwx): ORIGIN = RAM_START, LENGTH = RAM_SIZE

	LINKER_DT_REGIONS()
	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST (wx) : ORIGIN = 0xFFFFF780, LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{

#include <zephyr/linker/rel-sections.ld>

	GROUP_START(ROMABLE_REGION)
	. = ROM_START;   /* for kernel logging */
	PLACE_SYMBOL_HERE(__rodata_region_start);

	 .fvectors 0xFFFFFF80: AT(0xFFFFFF80)
	{
		KEEP(*(.fvectors))
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,ROM_START,)
	{
		_image_text_start = .;
		*(.text)
		*(.text.*)
		*(P)
		etext = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;

	#include <zephyr/linker/common-rom.ld>

	SECTION_PROLOGUE(.rvectors,,)
	{
		_rvectors_start = .;
		KEEP(*(.rvectors))
		_rvectors_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
	SECTION_PROLOGUE(init,,)
	{
		KEEP(*(.init))
		__preinit_array_start = .;
		KEEP(*(.preinit_array))
		__preinit_array_end = .;
		__init_array_start = (. + 3) & ~ 3;
		KEEP(*(.init_array))
		KEEP(*(SORT(.init_array.*)))
		__init_array_end = .;
		__fini_array_start = .;
		KEEP(*(.fini_array))
		KEEP(*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
	SECTION_PROLOGUE(fini,,)
	{
		KEEP(*(.fini))
	} GROUP_LINK_IN(ROMABLE_REGION)
	SECTION_PROLOGUE(got,,)
	{
		*(.got)
		*(.got.plt)
	} GROUP_LINK_IN(ROMABLE_REGION)
	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		*(.rodata)
		*(.rodata.*)
		*(C_1)
		*(C_2)
		*(C)
		_erodata = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
	SECTION_PROLOGUE(eh_frame_hdr,,)
	{
		*(.eh_frame_hdr)
	} GROUP_LINK_IN(ROMABLE_REGION)
	SECTION_PROLOGUE(eh_frame,,)
	{
		*(.eh_frame)
	} GROUP_LINK_IN(ROMABLE_REGION)
	SECTION_PROLOGUE(jcr,,)
	{
		*(.jcr)
	} GROUP_LINK_IN(ROMABLE_REGION)

	/* TODO: is this section necessary? There is a similar section
	 * (_CTOR_SECTION_NAME) in common-rom.ld. This seems to be for
	 * C++ Constructors/Destructors? */
	SECTION_PROLOGUE(tors,,)
	{
		__CTOR_LIST__ = .;
		. = ALIGN(2);
		__ctors = .;
		*(.ctors)
		__ctors_end = .;
		__CTOR_END__ = .;
		__DTOR_LIST__ = .;
		__dtors = .;
		*(.dtors)
		__dtors_end = .;
		__DTOR_END__ = .;
		. = ALIGN(2);
	} GROUP_LINK_IN(ROMABLE_REGION)

	PLACE_SYMBOL_HERE(__rodata_region_end);
	_mdata = .;
	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

	_image_ram_start = .;

	#if CONFIG_SRAM_BASE_ADDRESS == 0
		/* RX memory starts at address 0 which can be confused with NULL. To prevent this, block
		 * the first memory page (16 Bytes).
		 */
		SECTION_DATA_PROLOGUE(.null_blocker,,)
		{
			. = 0x10;
		} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
	#endif

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		_data = .;
		*(.data)
		*(.data.*)
		*(D)
		*(D_1)
		*(D_2)
		*(.gnu.linkonce.*)

#include <snippets-rwdata.ld>
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	/* the sections defined in common-ram.ld have to be initialized on
	 * reset as well, to place them before _edata */
#include <zephyr/linker/common-ram.ld>

	_edata = .;

	SECTION_PROLOGUE(_BSS_SECTION_NAME,,)
	{
		_bss = .;
		*(.bss)
		*(.bss.**)
		*(COMMON)
		*(B)
		*(B_1)
		*(B_2)
	} GROUP_LINK_IN(RAMABLE_REGION)

	_ebss = . ;

#include <zephyr/linker/common-noinit.ld>

	_image_ram_end = .;
	_end = .;
	PROVIDE(__end = _end);

	GROUP_END(RAMABLE_REGION)

/* Located in generated directory. This file is populated by the
* zephyr_linker_sources() CMake function.
*/
#include <snippets-sections.ld>

#include <zephyr/linker/debug-sections.ld>

}
