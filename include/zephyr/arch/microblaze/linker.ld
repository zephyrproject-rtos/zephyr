/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>
#include <layout.h>

/* These sections are specific to this CPU */
#define _EXCEPTION_SECTION_NAME exceptions
#define _RESET_SECTION_NAME reset

#define ROMABLE_REGION app_ram
#define RAMABLE_REGION app_ram

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

	GROUP_START(ROMABLE_REGION)

	SECTION_PROLOGUE(.text,,)
	{
		/* microblaze vectors from 0x0 to 0x50 */
		__mb_vectors = .;
		KEEP (*(.vectors.reset))
		. = __mb_vectors + 0x8;
		KEEP (*(.vectors.sw_exception))
		. = __mb_vectors + 0x10;
		KEEP (*(.vectors.interrupt))
		. = __mb_vectors + 0x20;
		KEEP (*(.vectors.hw_exception))

		/* code */
		. = __mb_vectors + 0x50;

		__text_region_start = .;

	   *(.text)
	   *(.text.*)
	   *(.gnu.linkonce.t.*)
	} GROUP_LINK_IN(RAMABLE_REGION)

	__text_region_end = .;
	__text_region_size = __text_region_end - __text_region_start;

	SECTION_DATA_PROLOGUE(_EXCEPTION_SECTION_NAME,,)
	{
		KEEP(*(".exception.entry.*"))
		*(".exception.other.*")

	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(.note.gnu.build-id,,)
	{
		KEEP (*(.note.gnu.build-id))
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(.init,,)
	{
		KEEP (*(.init))
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(.fini,,)
	{
		KEEP (*(.fini))
	} GROUP_LINK_IN(ROMABLE_REGION)

	__rodata_region_start = .;

#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
#include <zephyr/linker/thread-local-storage.ld>

	SECTION_PROLOGUE(.rodata,,)
	{
		. = ALIGN(4);
		__rodata_start = .;
		*(.rodata)
		*(.rodata.*)
		*(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

	} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>

	__rodata_end = .;
	__rodata_region_end = .;
	__rodata_region_size = __rodata_region_end - __rodata_region_start;

	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

	_image_ram_start = .;

#ifdef CONFIG_CPP
	/* These exist to keep Vitis Microblaze toolchain happy */
	SECTION_PROLOGUE(.ctors,,)
	{
		__CTOR_LIST__ = .;
		___CTORS_LIST___ = .;
		KEEP (*crtbegin.o(.ctors))
		KEEP (*(EXCLUDE_FILE(*crtend.o) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
		__CTOR_END__ = .;
		___CTORS_END___ = .;
	} GROUP_LINK_IN(RAMABLE_REGION)
	SECTION_PROLOGUE(.dtors,,)
	{
		__DTOR_LIST__ = .;
		___DTORS_LIST___ = .;
		KEEP (*crtbegin.o(.dtors))
		KEEP (*(EXCLUDE_FILE(*crtend.o) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
		PROVIDE(__DTOR_END__ = .);
		PROVIDE(___DTORS_END___ = .);
	} GROUP_LINK_IN(RAMABLE_REGION)
#endif

	SECTION_DATA_PROLOGUE(.sdata2,,)
	{
		. = ALIGN(4);
		__sdata2_start = .;
		*(  .sdata2)
		*(.sdata2.*)
		*(.gnu.linkonce.s2.*)
		. = ALIGN(4);
		__sdata2_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>
#include <zephyr/linker/cplusplus-ram.ld>

	SECTION_DATA_PROLOGUE(.data,,)
	{
		. = ALIGN(4);
		__data_start = .;
		*(.data)
		*(.data.*)

#include <snippets-rwdata.ld>

		*(.gnu.linkonce.d.*)
		. = ALIGN(4);

#include <snippets-ram-sections.ld>
	} GROUP_LINK_IN(RAMABLE_REGION)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

	__data_end = .;
	__data_size = __data_end - __data_start;

	SECTION_PROLOGUE(.got,,)
	{
		. = ALIGN(4);
		*(.got)
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(.got1,,)
	{
		. = ALIGN(4);
		*(.got1)
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(.got2,,)
	{
		. = ALIGN(4);
		*(.got2)
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(.jcr,,)
	{
		. = ALIGN(4);
		*(.jcr)
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_DATA_PROLOGUE(.sdata,,)
	{
		. = ALIGN(4);
		__sdata_start = .;
		*(.sdata)
		*(.sdata.*)
		*(.gnu.linkonce.s.*)

		__sdata_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(.sbss (NOLOAD),,)
	{
		. = ALIGN(4);
		__sbss_start = .;
		*(.sbss)
		*(.sbss.*)
		*(.gnu.linkonce.sb.*)
		. = ALIGN(4);
		__sbss_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	_gp = (__sbss_end + __sdata_start) / 2;
	PROVIDE(gp = _gp);

	SECTION_DATA_PROLOGUE(.tdata,,)
	{
		. = ALIGN(4);
		__tdata_start = .;
		*(.tdata)
		*(.tdata.*)
		*(.gnu.linkonce.td.*)
		__tdata_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(.tbss,,)
	{
		. = ALIGN(4);
		__tbss_start = .;
		*(.tbss)
		*(.tbss.*)
		*(.gnu.linkonce.tb.*)
		__tbss_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(.bss (NOLOAD),,)
	{
		. = ALIGN(4);
		__bss_start = .;
		*(.bss)
		*(.bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)


	_SDA_BASE_ = __sdata_start + ((__sbss_end - __sdata_start) / 2 );

	_SDA2_BASE_ = __sdata2_start + ((__sdata2_end - __sdata2_start) / 2 );

#include <zephyr/linker/common-noinit.ld>

#ifdef CONFIG_GEN_ISR_TABLES
#include <zephyr/linker/intlist.ld>
#endif

#include <snippets-sections.ld>

	/* Vitis Microblaze toolchain may moan without these sections,
	 * but we won't give them any space. Zephyr creates its malloc arena
	 * after _end. These are in case someone tries to use Vitis GCC with
	 * its _HEAP_SIZE and _STACK_SIZE definitions. e.g. during migration
	 * I'm making the linker moan so that you now know better than to rely
	 * on those values in Zephyr world.
	 */
	_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? -_HEAP_SIZE : 0;
	_STACK_SIZE = DEFINED(_STACK_SIZE) ? -_STACK_SIZE : 0;
	SECTION_PROLOGUE(.heap (NOLOAD),,)
	{
		. += _HEAP_SIZE;
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(.stack (NOLOAD),,)
	{
		. += _STACK_SIZE;
	} GROUP_LINK_IN(RAMABLE_REGION)

#include <zephyr/linker/ram-end.ld>

    GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

}
