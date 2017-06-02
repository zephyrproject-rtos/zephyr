/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common linker sections
 *
 * This script defines the memory location of the various sections that make up
 * a Zephyr Kernel image. This file is used by the linker.
 *
 * This script places the various sections of the image according to what
 * features are enabled by the kernel's configuration options.
 *
 * For a build that does not use the execute in place (XIP) feature, the script
 * generates an image suitable for loading into and executing from RAMABLE_REGION by
 * placing all the sections adjacent to each other.  There is also no separate
 * load address for the DATA section which means it doesn't have to be copied
 * into RAMABLE_REGION.
 *
 * For builds using XIP, there is a different load memory address (LMA) and
 * virtual memory address (VMA) for the DATA section.  In this case the DATA
 * section is copied into RAMABLE_REGION at runtime.
 *
 * When building an XIP image the data section is placed into ROMABLE_REGION.  In this
 * case, the LMA is set to __data_rom_start so the data section is concatenated
 * at the end of the RODATA section.  At runtime, the DATA section is copied
 * into the RAMABLE_REGION region so it can be accessed with read and write permission.
 *
 * Most symbols defined in the sections below are subject to be referenced in
 * the Zephyr Kernel image. If a symbol is used but not defined the linker will
 * emit an undefined symbol error.
 *
 * Please do not change the order of the section as the kernel expects this
 * order when programming the MMU.
 */

#define _LINKER

#define _ASMLANGUAGE
#include <linker-defs.h>
#include <offsets.h>
#include <misc/util.h>
#define MMU_PAGE_SIZE KB(4)

#include <linker-tool.h>

#ifdef CONFIG_XIP
	#define ROMABLE_REGION ROM
	#define RAMABLE_REGION RAM
#else
	#define ROMABLE_REGION RAM
	#define RAMABLE_REGION RAM
#endif

/* SECTIONS definitions */
SECTIONS
	{
	GROUP_START(ROMABLE_REGION)

	_image_rom_start = PHYS_LOAD_ADDR;
	_image_text_start = PHYS_LOAD_ADDR;


	SECTION_PROLOGUE(_TEXT_SECTION_NAME, (OPTIONAL),)
	{
	. = CONFIG_TEXT_SECTION_OFFSET;
	*(.text_start)
	*(".text_start.*")
	*(.text)
	*(".text.*")
	*(.gnu.linkonce.t.*)
	*(.eh_frame)
	*(.init)
	*(.fini)
	*(.eini)
	KEEP(*(.openocd_dbg))
	KEEP(*(".openocd_dbg.*"))
	KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;

#include <linker/common-rom.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME, (OPTIONAL),)
	{
	*(.rodata)
	*(".rodata.*")
	*(.gnu.linkonce.r.*)

	. = ALIGN(8);
	_idt_base_address = .;
#ifdef LINKER_PASS2
	KEEP(*(staticIdt))
#else
	. += CONFIG_IDT_NUM_VECTORS * 8;
#endif

#ifndef CONFIG_X86_FIXED_IRQ_MAPPING
	. = ALIGN(4);
	_irq_to_interrupt_vector = .;
#ifdef LINKER_PASS2
	KEEP(*(irq_int_vector_map))
#else
	. += CONFIG_MAX_IRQ_LINES;
#endif
#endif

#ifdef CONFIG_CUSTOM_RODATA_LD
/* Located in project source directory */
#include <custom-rodata.ld>
#endif

	KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_rom_end = .;
	__data_rom_start = ALIGN(4);		/* XIP imaged DATA ROMABLE_REGION start addr */

	GROUP_END(ROMABLE_REGION)

	/* RAMABLE_REGION */
	GROUP_START(RAMABLE_REGION)

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME, (OPTIONAL),)
	{
	KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	_image_ram_start = .;
	__data_ram_start = .;
	*(.top_of_image_ram)
	*(.top_of_image_ram.*)
	*(.data)
	*(".data.*")

#ifdef CONFIG_CUSTOM_RWDATA_LD
/* Located in project source directory */
#include <custom-rwdata.ld>
#endif

	. = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <linker/common-ram.ld>

	__data_ram_end = .;

	SECTION_PROLOGUE(_BSS_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
	/*
	 * For performance, BSS section is forced to be both 4 byte aligned and
	 * a multiple of 4 bytes.
	 */

	. = ALIGN(4);

	__bss_start = .;

	*(.bss)
	*(".bss.*")
	COMMON_SYMBOLS
	/*
	 * As memory is cleared in words only, it is simpler to ensure the BSS
	 * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
	 */
	. = ALIGN(4);
	__bss_end = .;
	KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	} GROUP_LINK_IN(RAMABLE_REGION)
#ifdef CONFIG_XIP
	/*
	 * Ensure linker keeps sections in correct order, despite the fact
	 * the previous section specified a load address and this no-load
	 * section doesn't.
	 */
	  GROUP_FOLLOWS_AT(RAMABLE_REGION)
#endif

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
	/*
	 * This section is used for non-initialized objects that
	 * will not be cleared during the boot process.
	 */
	*(.noinit)
	*(".noinit.*")

	*(.bottom_of_image_ram)
	*(.bottom_of_image_ram.*)
	} GROUP_LINK_IN(RAMABLE_REGION)


	/* Define linker symbols */
	_image_ram_end = .;
	_end = .; /* end of image */

	. = ALIGN(MMU_PAGE_SIZE);

	__bss_num_words	= (__bss_end - __bss_start) >> 2;

	GROUP_END(RAMABLE_REGION)

#ifndef LINKER_PASS2
	/* static interrupts */
	SECTION_PROLOGUE(intList, (OPTIONAL),)
	{
	KEEP(*(.spurIsr))
	KEEP(*(.spurNoErrIsr))
	__INT_LIST_START__ = .;
	LONG((__INT_LIST_END__ - __INT_LIST_START__) / __ISR_LIST_SIZEOF)
	KEEP(*(.intList))
	KEEP(*(.gnu.linkonce.intList.*))
	__INT_LIST_END__ = .;
	} > IDT_LIST
#else
	/DISCARD/ :
	{
	KEEP(*(.spurIsr))
	KEEP(*(.spurNoErrIsr))
	KEEP(*(.intList))
	KEEP(*(.gnu.linkonce.intList.*))
	}
#endif

#ifdef CONFIG_CUSTOM_SECTIONS_LD
/* Located in project source directory */
#include <custom-sections.ld>
#endif

	}

#ifdef CONFIG_XIP
/*
 * Round up number of words for DATA section to ensure that XIP copies the
 * entire data section. XIP copy is done in words only, so there may be up
 * to 3 extra bytes copied in next section (BSS). At run time, the XIP copy
 * is done first followed by clearing the BSS section.
 */
__data_size = (__data_ram_end - __data_ram_start);
__data_num_words = (__data_size + 3) >> 2;

#endif
