/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file for SPARC
 */

#include <soc.h>

#include <zephyr/linker/sections.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
    {

#include <zephyr/linker/rel-sections.ld>

    __rom_region_start = .;

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
		/* Trap table alignment required by SPARC V8 specification. */
		. = ALIGN(0x1000);
		__text_region_start = .;

		*(.text.traptable)
		*(.text)
		*(.text.*)
		*(.stub)
		*(.gnu.linkonce.t.*)
	} GROUP_LINK_IN(REGION_TEXT)

    __text_region_end = .;
    __rodata_region_start = .;

#include <zephyr/linker/common-rom.ld>
#include <zephyr/linker/thread-local-storage.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		 . = ALIGN(8);
		 *(.rodata)
		 *(.rodata.*)
		 *(.gnu.linkonce.r.*)
		 *(.rodata1)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

	} GROUP_LINK_IN(REGION_RODATA)

#include <zephyr/linker/cplusplus-rom.ld>

    __rodata_region_end = .;
    __rodata_region_size = __rodata_region_end - __rodata_region_start;

    __rom_region_end = .;

    __data_region_load_start = .;


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
		 . = ALIGN(8);
		 _image_ram_start = .;
		 __data_region_start = .;
		 __data_start = .;

		 *(.data)
		 *(.data.*)
		 *(.gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

		 __data_end = .;

	}  GROUP_DATA_LINK_IN(REGION_DATA_VMA, REGION_DATA_LMA)
    __data_size = __data_end - __data_start;
    __data_load_start = LOADADDR(_DATA_SECTION_NAME);

#include <zephyr/linker/common-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

    __data_region_end = .;

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		 . = ALIGN(4);
		 __bss_start = .;
		 *(.dynbss)
		 *(.bss)
		 *(.bss.*)
		 *(.gnu.linkonce.b.*)
		 COMMON_SYMBOLS
		 /*
		  * As memory is cleared in words only, it is simpler to ensure the BSS
		  * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		  */
		  __bss_end = ALIGN(4);
	}  GROUP_LINK_IN(REGION_BSS)

    SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * This section is used for non-initialized objects that
		 * will not be cleared during the boot process.
		 */
		 *(.noinit)
		 *(.noinit.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-noinit.ld>

	} GROUP_LINK_IN(REGION_BSS)

#include <zephyr/linker/cplusplus-ram.ld>

     _image_ram_end = .;
     _end = .; /* end of image */

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#include <zephyr/linker/debug-sections.ld>

    /DISCARD/ : { *(.note.GNU-stack) }
    /DISCARD/ : { *(.gnu_debuglink) }
    /DISCARD/ : { *(.gnu.lto_*) }

    SECTION_PROLOGUE(.gnu.attributes, 0,)
	{
	KEEP(*(.gnu.attributes))
	}

}
