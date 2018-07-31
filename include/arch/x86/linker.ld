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
#include <linker/linker-defs.h>
#include <offsets.h>
#include <misc/util.h>

#include <linker/linker-tool.h>

#ifdef CONFIG_XIP
	#define ROMABLE_REGION ROM
	#define RAMABLE_REGION RAM
#else
	#define ROMABLE_REGION RAM
	#define RAMABLE_REGION RAM
#endif

#ifdef CONFIG_X86_MMU
	#define MMU_PAGE_SIZE	KB(4)
	#define MMU_PAGE_ALIGN	. = ALIGN(MMU_PAGE_SIZE);
#else
	#define MMU_PAGE_ALIGN
#endif

ENTRY(CONFIG_KERNEL_ENTRY)

/* SECTIONS definitions */
SECTIONS
	{
	GROUP_START(ROMABLE_REGION)
#ifdef CONFIG_REALMODE
	/* 16-bit sections */
	. = PHYS_RAM_ADDR;

	SECTION_PROLOGUE(boot, (OPTIONAL),)
	{
	*(.boot)
	. = ALIGN(16);
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif
	. = ALIGN(8);

	_image_rom_start = PHYS_LOAD_ADDR;
#ifndef CONFIG_REALMODE
	_image_text_start = PHYS_LOAD_ADDR;
#else
	_image_text_start = .;
#endif
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

#include <linker/kobject-text.ld>

	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;
	_image_rodata_start = .;

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

#include <linker/kobject-rom.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_rodata_end = .;
	MMU_PAGE_ALIGN
#ifdef CONFIG_XIP
	/* Kernel ROM extends to the end of flash. Need to do this to program
	 * the MMU
	 */
	_image_rom_end = _image_rom_start + KB(CONFIG_ROM_SIZE);
#else
	/* ROM ends here, position counter will now be in RAM areas */
	_image_rom_end = .;
#endif
	_image_rom_size = _image_rom_end - _image_rom_start;

	GROUP_END(ROMABLE_REGION)

	/* RAMABLE_REGION */
	GROUP_START(RAMABLE_REGION)

	/* APP SHARED MEMORY REGION */
	SECTION_PROLOGUE(_APP_SMEM_SECTION_NAME, (OPTIONAL),)
	{
		_image_ram_start = .;
		_app_smem_start = .;
		APP_SMEM_SECTION()
		MMU_PAGE_ALIGN
		_app_smem_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#ifdef CONFIG_APPLICATION_MEMORY
	SECTION_DATA_PROLOGUE(_APP_DATA_SECTION_NAME, (OPTIONAL),)
	{
#ifndef CONFIG_XIP
		MMU_PAGE_ALIGN
#endif
		__app_ram_start = .;
		__app_data_ram_start = .;
		APP_INPUT_SECTION(.data)
		APP_INPUT_SECTION(".data.*")
		__app_data_ram_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__app_data_rom_start = LOADADDR(_APP_DATA_SECTION_NAME);

	SECTION_PROLOGUE(_APP_BSS_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
		__app_bss_start = .;
		APP_INPUT_SECTION(.bss)
		APP_INPUT_SECTION(".bss.*")
		APP_INPUT_SECTION(COMMON)
		__app_bss_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	__app_bss_num_words = (__app_bss_end - __app_bss_start) >> 2;

	SECTION_PROLOGUE(_APP_NOINIT_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
		APP_INPUT_SECTION(.noinit)
		APP_INPUT_SECTION(".noinit.*")
		MMU_PAGE_ALIGN
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	__app_ram_end = .;
	__app_ram_size = __app_ram_end - __app_ram_start;
#endif /* CONFIG_APPLICATION_MEMORY */

	SECTION_PROLOGUE(_BSS_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
	MMU_PAGE_ALIGN

	/*
	 * For performance, BSS section is forced to be both 4 byte aligned and
	 * a multiple of 4 bytes.
	 */
	. = ALIGN(4);
	__kernel_ram_start = .;
	__bss_start = .;

	KERNEL_INPUT_SECTION(.bss)
	KERNEL_INPUT_SECTION(".bss.*")
	KERNEL_INPUT_SECTION(COMMON)
	*(".kernel_bss.*")

	/*
	 * As memory is cleared in words only, it is simpler to ensure the BSS
	 * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
	 */
	. = ALIGN(4);
	__bss_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	__bss_num_words	= (__bss_end - __bss_start) >> 2;

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
	/*
	 * This section is used for non-initialized objects that
	 * will not be cleared during the boot process.
	 */
	KERNEL_INPUT_SECTION(.noinit)
	KERNEL_INPUT_SECTION(".noinit.*")
	*(".kernel_noinit.*")

	MMU_PAGE_ALIGN

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME, (OPTIONAL),)
	{

	__data_ram_start = .;

	KERNEL_INPUT_SECTION(.data)
	KERNEL_INPUT_SECTION(".data.*")
	*(".kernel.*")

#ifdef CONFIG_CUSTOM_RWDATA_LD
/* Located in project source directory */
#include <custom-rwdata.ld>
#endif

#ifdef CONFIG_GDT_DYNAMIC
	KEEP(*(.tss))
	. = ALIGN(8);
	_gdt = .;
#ifdef LINKER_PASS2
	KEEP(*(gdt_ram_data))
#else /* LINKER_PASS2 */

#ifdef CONFIG_USERSPACE
    #define GDT_NUM_ENTRIES 7
#elif defined(CONFIG_HW_STACK_PROTECTION)
    #define GDT_NUM_ENTRIES 5
#else
    #define GDT_NUM_ENTRIES 3
#endif /* CONFIG_X86_USERSPACE */
	. += GDT_NUM_ENTRIES * 8;
#endif /* LINKER_PASS2 */
#endif /* CONFIG_GDT_DYNAMIC */

	. = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__data_rom_start = LOADADDR(_DATA_SECTION_NAME);

#include <linker/common-ram.ld>

#ifdef CONFIG_X86_MMU
	/* Can't really predict the size of this section. Anything after this
	 * should not be affected if addresses change between builds (currently
	 * just the gperf tables which is fine).
	 *
	 * However, __mmu_tables_start *must* remain stable between builds,
	 * we can't have anything shifting the memory map beforehand.
	 */
	SECTION_DATA_PROLOGUE(mmu_tables, (OPTIONAL),)
	{
	/* Page Tables are located here if MMU is enabled.*/
	MMU_PAGE_ALIGN
	__mmu_tables_start = .;
	KEEP(*(.mmu_data));
	__mmu_tables_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

#include <linker/kobject.ld>

	MMU_PAGE_ALIGN
	__data_ram_end = .;

	/* All unused memory also owned by the kernel for heaps */
	__kernel_ram_end = PHYS_RAM_ADDR + KB(CONFIG_RAM_SIZE);
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

	_image_ram_end = .;
	_image_ram_all = (PHYS_RAM_ADDR + KB(CONFIG_RAM_SIZE)) - _image_ram_start;

	_end = .; /* end of image */

	GROUP_END(RAMABLE_REGION)

#ifndef LINKER_PASS2
	/* static interrupts */
	SECTION_PROLOGUE(intList, (OPTIONAL),)
	{
	KEEP(*(.spurIsr))
	KEEP(*(.spurNoErrIsr))
	KEEP(*(.intList))
	KEEP(*(.gnu.linkonce.intList.*))
	} > IDT_LIST

#ifdef CONFIG_X86_MMU
	/* Memory management unit*/
	SECTION_PROLOGUE(mmulist, (OPTIONAL),)
	{
	/* get size of the mmu lists needed for gen_mmu_x86.py*/
	LONG((__MMU_LIST_END__ - __MMU_LIST_START__) / __MMU_REGION_SIZEOF)
	/* Get the start of mmu tables in data section so that the address
	 * of the page tables can be calculated.
	 */
	LONG(__mmu_tables_start)
	__MMU_LIST_START__ = .;
	KEEP(*(.mmulist))
	__MMU_LIST_END__ = .;
	} > MMU_LIST
#endif /* CONFIG_X86_MMU */

#else
	/DISCARD/ :
	{
	KEEP(*(.spurIsr))
	KEEP(*(.spurNoErrIsr))
	KEEP(*(.intList))
	KEEP(*(.gnu.linkonce.intList.*))
	KEEP(*(.mmulist))
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
#ifdef CONFIG_APPLICATION_MEMORY
__app_data_size = (__app_data_ram_end - __app_data_ram_start);
__app_data_num_words = (__app_data_size + 3) >> 2;
#endif

#endif
