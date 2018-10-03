/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the pulpino platform
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define ROMABLE_REGION              DATARAM
#define RAMABLE_REGION              DATARAM

#define _VECTOR_SECTION_NAME        vector
#define _EXCEPTION_SECTION_NAME     exceptions
#define _RESET_SECTION_NAME         reset

ENTRY(__reset)

MEMORY
    {
    INSTRRAM (wx) : ORIGIN = CONFIG_ITCM_BASE_ADDRESS, LENGTH = CONFIG_ITCM_SIZE
    DATARAM  (rw) : ORIGIN = CONFIG_DTCM_BASE_ADDRESS, LENGTH = CONFIG_DTCM_SIZE
    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
    {
    GROUP_START(INSTRRAM)

    SECTION_PROLOGUE(_VECTOR_SECTION_NAME,,)
    {
		. = ALIGN(4);
		KEEP(*(.vectors.*))
    } GROUP_LINK_IN(INSTRRAM)

    SECTION_PROLOGUE(_RESET_SECTION_NAME,,)
    {
		KEEP(*(.reset.*))
    } GROUP_LINK_IN(INSTRRAM)

    SECTION_PROLOGUE(_EXCEPTION_SECTION_NAME,,)
    {
		 KEEP(*(".exception.entry.*"))
		 *(".exception.other.*")
    } GROUP_LINK_IN(INSTRRAM)

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
		. = ALIGN(4);

		KEEP(*(.openocd_dbg))
		KEEP(*(".openocd_dbg.*"))

		_image_text_start = .;
		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)
	} GROUP_LINK_IN(INSTRRAM)

    _image_text_end = .;

    GROUP_END(INSTRRAM)

    GROUP_START(RAMABLE_REGION)

#include <linker/common-rom.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		 . = ALIGN(4);
#ifdef CONFIG_GEN_SW_ISR_TABLE
		KEEP(*(SW_ISR_TABLE))
#endif
		*(.rodata)
		*(".rodata.*")
		*(.gnu.linkonce.r.*)
	} GROUP_LINK_IN(RAMABLE_REGION)

    _image_ram_start = .;

#include <linker/common-ram.ld>

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{

		 . = ALIGN(4);
		 *(.data)
		 *(".data.*")

		 *(.sdata .sdata.* .gnu.linkonce.s.*)
		 *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)

	}  GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		 . = ALIGN(4);
		 __bss_start = .;
		 *(.sbss)
		 *(".sbss.*")
		 *(.bss)
		 *(".bss.*")
		 COMMON_SYMBOLS
		 /*
		  * As memory is cleared in words only, it is simpler to ensure the BSS
		  * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		  */
		  __bss_end = ALIGN(4);
	}  GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * This section is used for non-initialized objects that
		 * will not be cleared during the boot process.
		 */
		 *(.noinit)
		 *(".noinit.*")
	} GROUP_LINK_IN(RAMABLE_REGION)

     _image_ram_end = .;
     _end = .; /* end of image */

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif

     GROUP_END(RAMABLE_REGION)
}
