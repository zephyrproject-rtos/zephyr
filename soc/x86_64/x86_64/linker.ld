/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <autoconf.h> /* Seems not to get picked up automatically? */

#define _LINKER
#define _ASMLANGUAGE
#include <linker/linker-defs.h>

/* The common-ram.ld definitions are written to a sort of oddball
 * preprocessor API which we reimplement here.  The default implementation
 * is incompatible with the simpler way we handle address assignment
 * and ELF segment definitions
 */
#define SORT_BY_NAME(x) SORT(x)
#define SECTION_DATA_PROLOGUE(name, opts, align) name opts : align
#define SECTION_PROLOGUE(name, opts, align)      name opts : align
#define GROUP_DATA_LINK_IN(v, l) :ram
#define GROUP_LINK_IN(a)         :ram

PHDRS {
      ram PT_LOAD;
}

_start = 0x100000;
ENTRY(_start);

SECTIONS {
	. = 0x100000;

	text : {
		KEEP(*(.xuk_stub32*))
		. = ALIGN(16);
		KEEP(*(.xuk_start64*))

		*(.text_start*)
		*(.text*)
		*(.gnu.linkonce.t.*)
		*(.eh_frame)
		*(.init)
		*(.fini)
		*(.eini)
		KEEP(*(.openocd_dbg*))
	} :ram

#include <linker/common-rom.ld>

	rodata : {
		*(.rodata*)
		*(.gnu.linkonce.r.*)
	} :ram

	datas : {
		*(.data*)
	} :ram

#include <linker/common-ram.ld>

	__bss_start = .;
	bss (NOLOAD) : {
		*(COMMON)
		*(.bss*)
	} :ram
	__bss_end = .;

	noinit (NOLOAD) : {
		*(.noinit*)
	} :ram


	/* We enable orphan section warnings, so these standard sections all
	 * have to be called out explicitly
	 */
	.comment 0 : { *(.comment) }
	.debug_frame 0 : { *(.debug_frame) }
	.debug_info 0 : { *(.debug_info) }
	.debug_abbrev 0 : { *(.debug_abbrev) }
	.debug_aranges 0 : { *(.debug_aranges) }
	.debug_ranges 0 : { *(.debug_ranges) }
	.debug_line 0 : { *(.debug_line) }
	.debug_str 0 : { *(.debug_str) }
	.debug_loc 0 : { *(.debug_loc) }
	.note.GNU-stack 0 : { *(.note.GNU-stack) }
	.picjunk 0 : { *(.got*) *(.*plt*) *(.rela.*) }
}
