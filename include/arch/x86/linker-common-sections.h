/* linker-common-sections.h - common linker sections */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This script defines the memory location of the various sections that make up
a Zephyr Kernel image. This file is used by the linker.

This script places the various sections of the image according to what features
are enabled by the kernel's configuration options.

For a build that does not use the execute in place (XIP) feature, the script
generates an image suitable for loading into and executing from RAM by placing
all the sections adjacent to each other.  There is also no separate load
address for the DATA section which means it doesn't have to be copied into RAM.

For builds using XIP, there is a different load memory address (LMA) and
virtual memory address (VMA) for the DATA section.  In this case the DATA
section is copied into RAM at runtime.

When building an XIP image the data section is placed into ROM.  In this case,
the LMA is set to __data_rom_start so the data section is concatenated at the
end of the RODATA section.  At runtime, the DATA section is copied into the RAM
region so it can be accessed with read and write permission.

Most symbols defined in the sections below are subject to be referenced in the
Zephyr Kernel image. If a symbol is used but not defined the linker will emit an
undefined symbol error.

Please do not change the order of the section as the nanokernel expects this
order when programming the MMU.
 */

#define _LINKER
#define _ASMLANGUAGE /* Needed to include mmustructs.h */

#include <linker-defs.h>
#include <offsets.h>
#include <misc/util.h>

#define MMU_PAGE_SIZE KB(4)

#include <linker-tool.h>

#define INIT_LEVEL(level)				\
		__initconfig##level##_start = .;	\
		*(.initconfig##level##.init)		\

/* SECTIONS definitions */
SECTIONS
	{
	GROUP_START(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME, (OPTIONAL),)
	{
	*(.text_start)
	*(".text_start.*")
	*(.text)
	*(".text.*")
	*(.eh_frame)
	*(.init)
	*(.fini)
	*(.eini)
	KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_CTOR_SECTION_NAME, (OPTIONAL),)
	{
	/*
	 * The compiler fills the constructor pointers table below, hence symbol
	 * __CTOR_LIST__ must be aligned on 4 byte boundary.
	 * To align with the C++ standard, the first element of the array
	 * contains the number of actual constructors. The last element is
	 * NULL.
	 */
	. = ALIGN(4);
	__CTOR_LIST__ = .;
	LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
	KEEP(*(SORT_BY_NAME(".ctors*")))
	LONG(0)
	__CTOR_END__ = .;
	KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE (devconfig, (OPTIONAL),)
	{
		__devconfig_start = .;
		*(".devconfig.*")
		KEEP(*(SORT_BY_NAME(".devconfig*")))
		__devconfig_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_RODATA_SECTION_NAME, (OPTIONAL),)
	{
	*(.rodata)
	*(".rodata.*")
	KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	} GROUP_LINK_IN(ROMABLE_REGION)

	__data_rom_start = ALIGN(4);		/* XIP imaged DATA ROM start addr */

	GROUP_END(ROMABLE_REGION)

	/* RAM */
	GROUP_START(RAM)

#if defined(CONFIG_XIP)
	SECTION_AT_PROLOGUE(_DATA_SECTION_NAME, (OPTIONAL),,__data_rom_start)
#else
	SECTION_PROLOGUE(_DATA_SECTION_NAME, (OPTIONAL),)
#endif
	{
	KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	__data_ram_start = .;
	*(.data)
	*(".data.*")
	IDT_MEMORY
	. = ALIGN(4);
	} GROUP_LINK_IN(RAM)

	SECTION_PROLOGUE (initlevel, (OPTIONAL),)
	{
		__initconfig_start = .;
		INIT_LEVEL(0)
		INIT_LEVEL(1)
		INIT_LEVEL(2)
		INIT_LEVEL(3)
		INIT_LEVEL(4)
		INIT_LEVEL(5)
		INIT_LEVEL(6)
		INIT_LEVEL(7)
		KEEP(*(SORT_BY_NAME(".initconfig*")))
		__initconfig_end = .;
		KEXEC_PGALIGN_PAD(MMU_PAGE_SIZE)
	} GROUP_LINK_IN(RAM)

	SECTION_PROLOGUE (_k_task_list, (OPTIONAL),)
	{
		_k_task_list_start = .;
			*(._k_task_list.public.*)
		_k_task_list_idle_start = .;
			*(._k_task_list.idle.*)
		KEEP(*(SORT_BY_NAME("._k_task_list*")))
		_k_task_list_end = .;
	} GROUP_LINK_IN(RAM)

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
	} GROUP_LINK_IN(RAM)
#ifdef CONFIG_XIP
	/*
	 * Ensure linker keeps sections in correct order, despite the fact
	 * the previous section specified a load address and this no-load
	 * section doesn't.
	 */
	  GROUP_FOLLOWS_AT(RAM)
#endif

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
	/*
	 * This section is used for non-intialized objects that
	 * will not be cleared during the boot process.
	 */
	*(.noinit)
	*(".noinit.*")

	INT_STUB_NOINIT
	} GROUP_LINK_IN(RAM)

	/* Define linker symbols */
	_end = .; /* end of image */

	. = ALIGN(MMU_PAGE_SIZE);

	__bss_num_words	= (__bss_end - __bss_start) >> 2;

	GROUP_END(RAM)

	/* static interrupts */
	SECTION_PROLOGUE(intList, (OPTIONAL),)
	{
	KEEP(*(.spurIsr))
	KEEP(*(.spurNoErrIsr))
	__INT_LIST_START__ = .;
	LONG((__INT_LIST_END__ - __INT_LIST_START__) / __ISR_LIST_SIZEOF)
	KEEP(*(.intList))
	__INT_LIST_END__ = .;
	} > IDT_LIST

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
